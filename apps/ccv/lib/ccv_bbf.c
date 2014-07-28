#include <enerc.h>
#include "ccv.h"
#include "ccv_internal.h"
#include <sys/time.h>
#ifdef HAVE_GSL
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#endif
#ifdef USE_OPENMP
#include <omp.h>
#endif

const ccv_bbf_param_t ccv_bbf_default_params = {
	.interval = 5,
	.min_neighbors = 2,
	.accurate = 1,
	.flags = 0,
	.size = {
		24,
		24,
	},
};

#define _ccv_width_padding(x) (((x) + 3) & -4)

static inline int _ccv_run_bbf_feature(ccv_bbf_feature_t* feature, int* step, APPROX unsigned char** u8)
{
#define pf_at(i) (*(u8[feature->pz[i]] + feature->px[i] + feature->py[i] * step[feature->pz[i]]))
#define nf_at(i) (*(u8[feature->nz[i]] + feature->nx[i] + feature->ny[i] * step[feature->nz[i]]))
	APPROX unsigned char pmin = pf_at(0), nmax = nf_at(0);
	/* check if every point in P > every point in N, and take a shortcut */
	if (ENDORSE(pmin <= nmax))
		return 0;
	int i;
	for (i = 1; i < ENDORSE(feature->size); i++)
	{
		if (ENDORSE(feature->pz[i]) >= 0)
		{
			int p = ENDORSE(pf_at(i));
			if (p < ENDORSE(pmin))
			{
				if (p <= ENDORSE(nmax))
					return 0;
				pmin = p;
			}
		}
		if (ENDORSE(feature->nz[i]) >= 0)
		{
			int n = ENDORSE(nf_at(i));
			if (n > ENDORSE(nmax))
			{
				if (ENDORSE(pmin) <= n)
					return 0;
				nmax = n;
			}
		}
	}
#undef pf_at
#undef nf_at
	return 1;
}

static int _ccv_read_bbf_stage_classifier(const char* file, ccv_bbf_stage_classifier_t* classifier)
{
	FILE* r = fopen(file, "r");
	if (r == 0) return -1;
	APPROX int stat = 0;
	stat |= fscanf(r, "%d", &classifier->count);
	union { float fl; int i; } fli;
	stat |= fscanf(r, "%d", &fli.i);
	classifier->threshold = fli.fl;
	classifier->feature = (ccv_bbf_feature_t*)ccmalloc(ENDORSE(classifier->count) * sizeof(ccv_bbf_feature_t));
	classifier->alpha = (float*)DEDORSE(ccmalloc(ENDORSE(classifier->count) * 2 * sizeof(float)));
	int i, j;
	for (i = 0; i < ENDORSE(classifier->count); i++)
	{
		stat |= fscanf(r, "%d", &classifier->feature[i].size);
		for (j = 0; j < ENDORSE(classifier->feature[i].size); j++)
		{
			stat |= fscanf(r, "%d %d %d", &classifier->feature[i].px[j], &classifier->feature[i].py[j], &classifier->feature[i].pz[j]);
			stat |= fscanf(r, "%d %d %d", &classifier->feature[i].nx[j], &classifier->feature[i].ny[j], &classifier->feature[i].nz[j]);
		}
		union { float fl; int i; } flia, flib;
		stat |= fscanf(r, "%d %d", &flia.i, &flib.i);
		classifier->alpha[i * 2] = flia.fl;
		classifier->alpha[i * 2 + 1] = flib.fl;
	}
	fclose(r);
	return 0;
}

void ccv_bbf_classifier_cascade_new(ccv_dense_matrix_t** posimg, int posnum, char** bgfiles, int bgnum, int negnum, ccv_size_t size, const char* dir, ccv_bbf_new_param_t params)
{
	fprintf(stderr, " ccv_bbf_classifier_cascade_new requires libgsl support, please compile ccv with libgsl.\n");
}

static int _ccv_is_equal(const void* _r1, const void* _r2, void* data)
{
	const ccv_comp_t* r1 = (const ccv_comp_t*)_r1;
	const ccv_comp_t* r2 = (const ccv_comp_t*)_r2;
	APPROX int distance = (int)(r1->rect.width * 0.25 + 0.5);

	return ENDORSE(r2->rect.x <= r1->rect.x + distance &&
		   r2->rect.x >= r1->rect.x - distance &&
		   r2->rect.y <= r1->rect.y + distance &&
		   r2->rect.y >= r1->rect.y - distance &&
		   r2->rect.width <= (int)(r1->rect.width * 1.5 + 0.5) &&
		   (int)(r2->rect.width * 1.5 + 0.5) >= r1->rect.width);
}

static int _ccv_is_equal_same_class(const void* _r1, const void* _r2, void* data)
{
	const ccv_comp_t* r1 = (const ccv_comp_t*)_r1;
	const ccv_comp_t* r2 = (const ccv_comp_t*)_r2;
	APPROX int distance = (int)(r1->rect.width * 0.25 + 0.5);

	return ENDORSE(r2->classification.id == r1->classification.id &&
		   r2->rect.x <= r1->rect.x + distance &&
		   r2->rect.x >= r1->rect.x - distance &&
		   r2->rect.y <= r1->rect.y + distance &&
		   r2->rect.y >= r1->rect.y - distance &&
		   r2->rect.width <= (int)(r1->rect.width * 1.5 + 0.5) &&
		   (int)(r2->rect.width * 1.5 + 0.5) >= r1->rect.width);
}

ccv_array_t* ccv_bbf_detect_objects(ccv_dense_matrix_t* a, ccv_bbf_classifier_cascade_t** _cascade, int count, ccv_bbf_param_t params)
{
	int hr = a->rows / ENDORSE(params.size.height);
	int wr = a->cols / ENDORSE(params.size.width);
	double scale = pow(2., 1. / (params.interval + 1.));
	int next = params.interval + 1;
	int scale_upto = (int)(log((double)ccv_min(hr, wr)) / log(scale));
	ccv_dense_matrix_t** pyr = (ccv_dense_matrix_t**)alloca((scale_upto + next * 2) * 4 * sizeof(ccv_dense_matrix_t*));
	memset(pyr, 0, (scale_upto + next * 2) * 4 * sizeof(ccv_dense_matrix_t*));
	if (ENDORSE(params.size.height != _cascade[0]->size.height || params.size.width != _cascade[0]->size.width))
		ccv_resample(a, &pyr[0], 0, a->rows * ENDORSE(_cascade[0]->size.height / params.size.height), a->cols * ENDORSE(_cascade[0]->size.width / params.size.width), CCV_INTER_AREA);
	else
		pyr[0] = a;
	int i, j, k, t, x, y, q;
	for (i = 1; i < ccv_min(params.interval + 1, scale_upto + next * 2); i++)
		ccv_resample(pyr[0], &pyr[i * 4], 0, (int)(pyr[0]->rows / pow(scale, i)), (int)(pyr[0]->cols / pow(scale, i)), CCV_INTER_AREA);
	for (i = next; i < scale_upto + next * 2; i++)
		ccv_sample_down(pyr[i * 4 - next * 4], &pyr[i * 4], 0, 0, 0);
	if (params.accurate)
		for (i = next * 2; i < scale_upto + next * 2; i++)
		{
			ccv_sample_down(pyr[i * 4 - next * 4], &pyr[i * 4 + 1], 0, 1, 0);
			ccv_sample_down(pyr[i * 4 - next * 4], &pyr[i * 4 + 2], 0, 0, 1);
			ccv_sample_down(pyr[i * 4 - next * 4], &pyr[i * 4 + 3], 0, 1, 1);
		}
	ccv_array_t* idx_seq;
	ccv_array_t* seq = ccv_array_new(sizeof(ccv_comp_t), 64, 0);
	ccv_array_t* seq2 = ccv_array_new(sizeof(ccv_comp_t), 64, 0);
	ccv_array_t* result_seq = ccv_array_new(sizeof(ccv_comp_t), 64, 0);
	/* detect in multi scale */
	for (t = 0; t < count; t++)
	{
		ccv_bbf_classifier_cascade_t* cascade = _cascade[t];
		APPROX float scale_x = (float) params.size.width / (float) cascade->size.width;
		APPROX float scale_y = (float) params.size.height / (float) cascade->size.height;
		ccv_array_clear(seq);
		for (i = 0; i < scale_upto; i++)
		{
			int dx[] = {0, 1, 0, 1};
			int dy[] = {0, 0, 1, 1};
			int i_rows = pyr[i * 4 + next * 8]->rows - ENDORSE(cascade->size.height >> 2);
			int steps[] = { pyr[i * 4]->step, pyr[i * 4 + next * 4]->step, pyr[i * 4 + next * 8]->step };
			int i_cols = pyr[i * 4 + next * 8]->cols - ENDORSE(cascade->size.width >> 2);
			int paddings[] = { pyr[i * 4]->step * 4 - i_cols * 4,
							   pyr[i * 4 + next * 4]->step * 2 - i_cols * 2,
							   pyr[i * 4 + next * 8]->step - i_cols };
			for (q = 0; q < (params.accurate ? 4 : 1); q++)
			{
				APPROX unsigned char* u8[] = { pyr[i * 4]->data.u8 + dx[q] * 2 + dy[q] * pyr[i * 4]->step * 2, pyr[i * 4 + next * 4]->data.u8 + dx[q] + dy[q] * pyr[i * 4 + next * 4]->step, pyr[i * 4 + next * 8 + q]->data.u8 };
				for (y = 0; y < i_rows; y++)
				{
					for (x = 0; x < i_cols; x++)
					{
						APPROX float sum;
						APPROX int flag = 1;
						ccv_bbf_stage_classifier_t* classifier = cascade->stage_classifier;
						for (j = 0; j < ENDORSE(cascade->count); ++j, ++classifier)
						{
							sum = 0;
							APPROX float* alpha = classifier->alpha;
							ccv_bbf_feature_t* feature = classifier->feature;
							for (k = 0; k < ENDORSE(classifier->count); ++k, alpha += 2, ++feature)
								sum += alpha[_ccv_run_bbf_feature(feature, steps, u8)];
							if (ENDORSE(sum) < ENDORSE(classifier->threshold))
							{
								flag = 0;
								break;
							}
						}
						if (ENDORSE(flag))
						{
							ccv_comp_t comp;
							comp.rect = ccv_rect((int)((x * 4 + dx[q] * 2) * scale_x + 0.5), (int)((y * 4 + dy[q] * 2) * scale_y + 0.5), (int)(cascade->size.width * scale_x + 0.5), (int)(cascade->size.height * scale_y + 0.5));
							comp.neighbors = 1;
							comp.classification.id = t;
							comp.classification.confidence = sum;
							ccv_array_push(seq, &comp);
						}
						u8[0] += 4;
						u8[1] += 2;
						u8[2] += 1;
					}
					u8[0] += paddings[0];
					u8[1] += paddings[1];
					u8[2] += paddings[2];
				}
			}
			scale_x *= scale;
			scale_y *= scale;
		}

		/* the following code from OpenCV's haar feature implementation */
		if(params.min_neighbors == 0)
		{
			for (i = 0; i < seq->rnum; i++)
			{
				ccv_comp_t* comp = (ccv_comp_t*)ccv_array_get(seq, i);
				ccv_array_push(result_seq, comp);
			}
		} else {
			idx_seq = 0;
			ccv_array_clear(seq2);
			// group retrieved rectangles in order to filter out noise
			int ncomp = ccv_array_group(seq, &idx_seq, _ccv_is_equal_same_class, 0);
			ccv_comp_t* comps = (ccv_comp_t*)ccmalloc((ncomp + 1) * sizeof(ccv_comp_t));
			memset(comps, 0, (ncomp + 1) * sizeof(ccv_comp_t));

			// count number of neighbors
			for(i = 0; i < seq->rnum; i++)
			{
				ccv_comp_t r1 = *(ccv_comp_t*)ccv_array_get(seq, i);
				int idx = *(int*)ccv_array_get(idx_seq, i);

				if (ENDORSE(comps[idx].neighbors) == 0)
					comps[idx].classification.confidence = r1.classification.confidence;

				++comps[idx].neighbors;

				comps[idx].rect.x += r1.rect.x;
				comps[idx].rect.y += r1.rect.y;
				comps[idx].rect.width += r1.rect.width;
				comps[idx].rect.height += r1.rect.height;
				comps[idx].classification.id = r1.classification.id;
				comps[idx].classification.confidence = ccv_max(comps[idx].classification.confidence, r1.classification.confidence);
			}

			// calculate average bounding box
			for(i = 0; i < ncomp; i++)
			{
				int n = ENDORSE(comps[i].neighbors);
				if(n >= params.min_neighbors)
				{
					ccv_comp_t comp;
					comp.rect.x = (comps[i].rect.x * 2 + n) / (2 * n);
					comp.rect.y = (comps[i].rect.y * 2 + n) / (2 * n);
					comp.rect.width = (comps[i].rect.width * 2 + n) / (2 * n);
					comp.rect.height = (comps[i].rect.height * 2 + n) / (2 * n);
					comp.neighbors = comps[i].neighbors;
					comp.classification.id = comps[i].classification.id;
					comp.classification.confidence = comps[i].classification.confidence;
					ccv_array_push(seq2, &comp);
				}
			}

			// filter out small face rectangles inside large face rectangles
			for(i = 0; i < seq2->rnum; i++)
			{
				ccv_comp_t r1 = *(ccv_comp_t*)ccv_array_get(seq2, i);
				APPROX int flag = 1;

				for(j = 0; j < seq2->rnum; j++)
				{
					ccv_comp_t r2 = *(ccv_comp_t*)ccv_array_get(seq2, j);
					APPROX int distance = (int)(r2.rect.width * 0.25 + 0.5);

					if(i != j &&
					   ENDORSE(r1.classification.id == r2.classification.id &&
					   r1.rect.x >= r2.rect.x - distance &&
					   r1.rect.y >= r2.rect.y - distance &&
					   r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					   r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					   (r2.neighbors > ccv_max(3, r1.neighbors) || r1.neighbors < 3)))
					{
						flag = 0;
						break;
					}
				}

				if(ENDORSE(flag))
					ccv_array_push(result_seq, &r1);
			}
			ccv_array_free(idx_seq);
			ccfree(comps);
		}
	}

	ccv_array_free(seq);
	ccv_array_free(seq2);

	ccv_array_t* result_seq2;
	/* the following code from OpenCV's haar feature implementation */
	if (params.flags & CCV_BBF_NO_NESTED)
	{
		result_seq2 = ccv_array_new(sizeof(ccv_comp_t), 64, 0);
		idx_seq = 0;
		// group retrieved rectangles in order to filter out noise
		int ncomp = ccv_array_group(result_seq, &idx_seq, _ccv_is_equal, 0);
		ccv_comp_t* comps = (ccv_comp_t*)ccmalloc((ncomp + 1) * sizeof(ccv_comp_t));
		memset(comps, 0, (ncomp + 1) * sizeof(ccv_comp_t));

		// count number of neighbors
		for(i = 0; i < result_seq->rnum; i++)
		{
			ccv_comp_t r1 = *(ccv_comp_t*)ccv_array_get(result_seq, i);
			int idx = *(int*)ccv_array_get(idx_seq, i);

			if (ENDORSE(comps[idx].neighbors == 0 || comps[idx].classification.confidence < r1.classification.confidence))
			{
				comps[idx].classification.confidence = r1.classification.confidence;
				comps[idx].neighbors = 1;
				comps[idx].rect = r1.rect;
				comps[idx].classification.id = r1.classification.id;
			}
		}

		// calculate average bounding box
		for(i = 0; i < ncomp; i++)
			if(ENDORSE(comps[i].neighbors))
				ccv_array_push(result_seq2, &comps[i]);

		ccv_array_free(result_seq);
		ccfree(comps);
	} else {
		result_seq2 = result_seq;
	}

	for (i = 1; i < scale_upto + next * 2; i++)
		ccv_matrix_free(pyr[i * 4]);
	if (params.accurate)
		for (i = next * 2; i < scale_upto + next * 2; i++)
		{
			ccv_matrix_free(pyr[i * 4 + 1]);
			ccv_matrix_free(pyr[i * 4 + 2]);
			ccv_matrix_free(pyr[i * 4 + 3]);
		}
	if (ENDORSE(params.size.height != _cascade[0]->size.height || params.size.width != _cascade[0]->size.width))
		ccv_matrix_free(pyr[0]);

	return result_seq2;
}

ccv_bbf_classifier_cascade_t* ccv_bbf_read_classifier_cascade(const char* directory)
{
	char buf[1024];
	sprintf(buf, "%s/cascade.txt", directory);
	int s, i;
	FILE* r = fopen(buf, "r");
	if (r == 0)
		return 0;
	ccv_bbf_classifier_cascade_t* cascade = (ccv_bbf_classifier_cascade_t*)ccmalloc(sizeof(ccv_bbf_classifier_cascade_t));
	s = fscanf(r, "%d %d %d", &cascade->count, &cascade->size.width, &cascade->size.height);
	assert(s > 0);
	cascade->stage_classifier = (ccv_bbf_stage_classifier_t*)ccmalloc(ENDORSE(cascade->count) * sizeof(ccv_bbf_stage_classifier_t));
	for (i = 0; i < ENDORSE(cascade->count); i++)
	{
		sprintf(buf, "%s/stage-%d.txt", directory, i);
		if (_ccv_read_bbf_stage_classifier(buf, &cascade->stage_classifier[i]) < 0)
		{
			cascade->count = i;
			break;
		}
	}
	fclose(r);
	return cascade;
}

ccv_bbf_classifier_cascade_t* ccv_bbf_classifier_cascade_read_binary(char* s)
{
	int i;
	ccv_bbf_classifier_cascade_t* cascade = (ccv_bbf_classifier_cascade_t*)ccmalloc(sizeof(ccv_bbf_classifier_cascade_t));
        APPROX int* count_ptr = DEDORSE(&cascade->count);
	memcpy(count_ptr, s, sizeof(cascade->count)); s += sizeof(cascade->count);
	memcpy(&cascade->size.width, s, sizeof(cascade->size.width)); s += sizeof(cascade->size.width);
	memcpy(&cascade->size.height, s, sizeof(cascade->size.height)); s += sizeof(cascade->size.height);
	ccv_bbf_stage_classifier_t* classifier = cascade->stage_classifier = (ccv_bbf_stage_classifier_t*)ccmalloc(ENDORSE(cascade->count) * sizeof(ccv_bbf_stage_classifier_t));
	for (i = 0; i < ENDORSE(cascade->count); i++, classifier++)
	{
		memcpy(&classifier->count, s, sizeof(classifier->count)); s += sizeof(classifier->count);
		memcpy(&classifier->threshold, s, sizeof(classifier->threshold)); s += sizeof(classifier->threshold);
		classifier->feature = (ccv_bbf_feature_t*)ccmalloc(ENDORSE(classifier->count) * sizeof(ccv_bbf_feature_t));
		classifier->alpha = (float*)ccmalloc(ENDORSE(classifier->count) * 2 * sizeof(float));
		memcpy(classifier->feature, s, classifier->count * sizeof(ccv_bbf_feature_t)); s += classifier->count * sizeof(ccv_bbf_feature_t);
		memcpy(classifier->alpha, s, classifier->count * 2 * sizeof(float)); s += classifier->count * 2 * sizeof(float);
	}
	return cascade;

}

int ccv_bbf_classifier_cascade_write_binary(ccv_bbf_classifier_cascade_t* cascade, APPROX char* s, int slen)
{
	int i;
	APPROX int len = sizeof(cascade->count) + sizeof(cascade->size.width) + sizeof(cascade->size.height);
	ccv_bbf_stage_classifier_t* classifier = cascade->stage_classifier;
	for (i = 0; i < ENDORSE(cascade->count); i++, classifier++)
		len += sizeof(classifier->count) + sizeof(classifier->threshold) + classifier->count * sizeof(ccv_bbf_feature_t) + classifier->count * 2 * sizeof(float);
	if (slen >= ENDORSE(len))
	{
                APPROX int* count_ptr = DEDORSE(&cascade->count);
		memcpy(s, count_ptr, sizeof(cascade->count)); s += sizeof(cascade->count);
		memcpy(s, &cascade->size.width, sizeof(cascade->size.width)); s += sizeof(cascade->size.width);
		memcpy(s, &cascade->size.height, sizeof(cascade->size.height)); s += sizeof(cascade->size.height);
		classifier = cascade->stage_classifier;
		for (i = 0; i < ENDORSE(cascade->count); i++, classifier++)
		{
			memcpy(s, &classifier->count, sizeof(classifier->count)); s += sizeof(classifier->count);
			memcpy(s, &classifier->threshold, sizeof(classifier->threshold)); s += sizeof(classifier->threshold);
			memcpy(s, classifier->feature, classifier->count * sizeof(ccv_bbf_feature_t)); s += classifier->count * sizeof(ccv_bbf_feature_t);
			memcpy(s, classifier->alpha, classifier->count * 2 * sizeof(float)); s += classifier->count * 2 * sizeof(float);
		}
	}
	return ENDORSE(len);
}

void ccv_bbf_classifier_cascade_free(ccv_bbf_classifier_cascade_t* cascade)
{
	int i;
	for (i = 0; i < ENDORSE(cascade->count); ++i)
	{
		ccfree(cascade->stage_classifier[i].feature);
		ccfree(cascade->stage_classifier[i].alpha);
	}
	ccfree(cascade->stage_classifier);
	ccfree(cascade);
}
