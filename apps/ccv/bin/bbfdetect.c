#include "ccv.h"
#include <sys/time.h>
#include <ctype.h>
#include <enerc.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int get_current_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char** argv)
{
        FILE* file;
        char *output_file = "my_output.txt";
        int i, ret_val;
        ccv_dense_matrix_t* image = 0;
        ccv_array_t* seq;

        accept_roi_begin();
	assert(argc >= 3);
	ccv_enable_default_cache();
	ccv_bbf_classifier_cascade_t* cascade = ccv_bbf_read_classifier_cascade(argv[2]);
	ccv_read(argv[1], &image, CCV_IO_GRAY | CCV_IO_ANY_FILE);
	if (image != 0)
	{
		unsigned int elapsed_time = get_current_time();
		seq = ccv_bbf_detect_objects(image, &cascade, 1, ccv_bbf_default_params);
		elapsed_time = get_current_time() - elapsed_time;
		for (i = 0; i < seq->rnum; i++)
		{
			ccv_comp_t* comp = (ccv_comp_t*)ccv_array_get(seq, i);
			printf("%d %d %d %d %f\n", comp->rect.x, comp->rect.y, comp->rect.width, comp->rect.height, comp->classification.confidence);
		}
		printf("total : %d in time %dms\n", seq->rnum, elapsed_time);
                ccv_bbf_classifier_cascade_free(cascade);
                ccv_disable_cache();
                accept_roi_end();
                
                file = fopen(output_file, "w");
                if (file == NULL) {
                  perror("fopen for write failed");
                  return EXIT_FAILURE;
                }
                // latest changes
                struct coordinates {
                    APPROX int x;
                    APPROX int y;
                };
                for (i = 0; i < seq->rnum; i++) {
                  ccv_comp_t* comp = (ccv_comp_t*) ccv_array_get(seq, i);
                  struct coordinates upperleft, upperright, lowerleft, lowerright;
                  upperleft.x = comp->rect.x;
                  upperleft.y = comp->rect.y;
                  upperright.x = comp->rect.x + comp->rect.width;
                  upperright.y = upperleft.y;
                  lowerleft.x = upperleft.x;
                  lowerleft.y = upperleft.y + comp->rect.height;
                  lowerright.x = upperright.x;
                  lowerright.y = lowerleft.y;
                  ret_val = fprintf(file, "%d %d\n%d %d\n%d %d\n%d %d\n", upperleft.x, upperleft.y, upperright.x, upperright.y,
                      lowerright.x, lowerright.y, lowerleft.x, lowerleft.y);
                  // latest changes
                  if (ret_val < 0) {
                    perror("fprintf of coordinates failed");
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
			}
			free(file);
			fclose(r);
		}
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
	}
	return 0;
}
