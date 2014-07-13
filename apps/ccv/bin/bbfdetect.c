#include "ccv.h"
#include <sys/time.h>
#include <ctype.h>

// added block
#include <enerc.h>
#include <stdio.h>
#include <stdlib.h>
// added block

unsigned int get_current_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char** argv)
{
        // added block
        FILE* file;
        char *output_file = "my_output.txt";
        int i, ret_val;
        ccv_dense_matrix_t* image = 0;
        ccv_array_t* seq;
        // added block

        // added line
        accept_roi_begin();
	assert(argc >= 3);
	// int i; removed line
	ccv_enable_default_cache();
	// ccv_dense_matrix_t* image = 0; removed line
	ccv_bbf_classifier_cascade_t* cascade = ccv_bbf_read_classifier_cascade(argv[2]);
	ccv_read(argv[1], &image, CCV_IO_GRAY | CCV_IO_ANY_FILE);
	if (image != 0)
	{
		unsigned int elapsed_time = get_current_time();
		seq = ccv_bbf_detect_objects(image, &cascade, 1, ccv_bbf_default_params); // seq already declared above
		elapsed_time = get_current_time() - elapsed_time;
		for (i = 0; i < seq->rnum; i++)
		{
			ccv_comp_t* comp = (ccv_comp_t*)ccv_array_get(seq, i);
			printf("%d %d %d %d %f\n", comp->rect.x, comp->rect.y, comp->rect.width, comp->rect.height, comp->classification.confidence);
		}
		printf("total : %d in time %dms\n", seq->rnum, elapsed_time);
		// ccv_array_free(seq); removed line
		// ccv_matrix_free(image); removed line

                // added block
                ccv_bbf_classifier_cascade_free(cascade);
                ccv_disable_cache();
                accept_roi_end();
                file = fopen(output_file, "w");
                if (file == NULL) {
                  perror("fopen for write failed");
                  return EXIT_FAILURE;
                }
                for (i = 0; i < seq->rnum; i++) {
                  ccv_comp_t* comp = (ccv_comp_t*) ccv_array_get(seq, i);
                  ret_val = fprintf(file, "%d\n%d\n%d\n%d\n%f\n", comp->rect.x, comp->rect.y, comp->rect.width,
                      comp->rect.height, comp->classification.confidence);
                  if (ret_val < 0) {
                    perror("fprintf of coordinates and confidence failed");
                    fclose(file);
                    return EXIT_FAILURE;
                  }
                }
                ret_val = fprintf(file, "%d\n%d\n", seq->rnum, elapsed_time);
                if (ret_val < 0) {
                  perror("fprintf of number and time failed");
                  fclose(file);
                  return EXIT_FAILURE;
                }
                ret_val = fclose(file);
                if (ret_val != 0) {
                  perror("fclose failed");
                  return EXIT_FAILURE;
                }
                ccv_array_free(seq);
                ccv_matrix_free(image);
                // added block

	} else {
		FILE* r = fopen(argv[1], "rt");
		if (argc == 4)
			chdir(argv[3]);
		if(r)
		{
			size_t len = 1024;
			char* file = (char*)malloc(len);
			ssize_t read;
			while((read = getline(&file, &len, r)) != -1)
			{
				while(read > 1 && isspace(file[read - 1]))
					read--;
				file[read] = 0;
				image = 0;
				ccv_read(file, &image, CCV_IO_GRAY | CCV_IO_ANY_FILE);
				assert(image != 0);
				seq = ccv_bbf_detect_objects(image, &cascade, 1, ccv_bbf_default_params); // seq already declared above
				for (i = 0; i < seq->rnum; i++)
				{
					ccv_comp_t* comp = (ccv_comp_t*)ccv_array_get(seq, i);
					printf("%s %d %d %d %d %f\n", file, comp->rect.x, comp->rect.y, comp->rect.width, comp->rect.height, comp->classification.confidence);
				}
				// ccv_array_free(seq); removed line
				// ccv_matrix_free(image); removed line
			}
			free(file);
			fclose(r);
		}

                // added block
                ccv_bbf_classifier_cascade_free(cascade);
                ccv_disable_cache();
                accept_roi_end();
                file = fopen(output_file, "w");
                if (file == NULL) {
                  perror("fopen for write failed");
                  return EXIT_FAILURE;
                }
                for (i = 0; i < seq->rnum; i++) {
                  ccv_comp_t* comp = (ccv_comp_t*) ccv_array_get(seq, i);
                  ret_val = fprintf(file, "%d\n%d\n%d\n%d\n%f\n", comp->rect.x, comp->rect.y, comp->rect.width,
                      comp->rect.height, comp->classification.confidence);
                  if (ret_val < 0) {
                    perror("fprintf of coordinates and confidence failed");
                    fclose(file);
                    return EXIT_FAILURE;
                  }
                }
                ret_val = fclose(file);
                if (ret_val != 0) {
                  perror("fclose failed");
                  return EXIT_FAILURE;
                }
                ccv_array_free(seq);
                ccv_matrix_free(image);
                // added block
	}
	// ccv_bbf_classifier_cascade_free(cascade); removed line
	// ccv_disable_cache(); removed line
	return 0;
}
