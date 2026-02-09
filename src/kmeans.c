// gcc -O0 -Wall -Werror kmeans.c -ljpeg -lm
// gcc -O3 -Wall -Werror kmeans.c -ljpeg -lm
// time ./a.out kitty.jpg kittyClusters.jpg 5
// time ./a.out kitty.jpg kittyClusters.jpg 10
// time ./a.out kitty.jpg kittyClusters.jpg 20
// perf stat -e instructions:u,cycles:u taskset -c 0 ./a.out bigImage.jpg bigImageClusters20.jpg 20
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <jpeglib.h> // Requires libjpeg
#include <limits.h>

#define ID 1069956
#define MAX_ITERATIONS 100
// --- Data Structures ---
// Structure to represent a pixel (Color in RGB space)
typedef struct {
    unsigned int r, g, b;
    unsigned int cluster_id;
} Pixel;

// Structure for Cluster Centers (Centroids)
typedef struct {
    unsigned int r, g, b;
    unsigned int pixel_count;
} Centroid;

// --- Helper: Color Palette ---
// Assigns a high-contrast color based on the cluster ID.
// Updated to support 20 distinct colors.
void getBasicColor(int cluster_id, unsigned int *r, unsigned int *g, unsigned int *b){
    // Palette of 20 distinct colors
    unsigned char palette[20][3] = {
        {255, 0, 0},     // 0: Red
        {0, 255, 0},     // 1: Green
        {0, 0, 255},     // 2: Blue
        {255, 255, 0},   // 3: Yellow
        {0, 255, 255},   // 4: Cyan
        {255, 0, 255},   // 5: Magenta
        {255, 165, 0},   // 6: Orange
        {128, 0, 128},   // 7: Purple
        {255, 192, 203}, // 8: Pink
        {165, 42, 42},   // 9: Brown
        {128, 128, 128}, // 10: Gray
        {0, 0, 0},       // 11: Black
        {255, 255, 255}, // 12: White
        {128, 0, 0},     // 13: Maroon
        {0, 128, 0},     // 14: Dark Green
        {0, 0, 128},     // 15: Navy
        {128, 128, 0},   // 16: Olive
        {0, 128, 128},   // 17: Teal
        {255, 127, 80},  // 18: Coral
        {189, 183, 107}  // 19: Dark Khaki
    };

    // Use modulo to cycle through colors if K > 20
    int idx = cluster_id % 20;

    *r = palette[idx][0];
    *g = palette[idx][1];
    *b = palette[idx][2];
}

Pixel* read_jpeg_file(const char *filename, int *width, int *height) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *infile;
    JSAMPARRAY buffer;

    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Error: Can't open %s\n", filename);
        exit(1);
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    // Αποθήκευση Metadata
    jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);
    jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    long total_pixels = (long)(*width) * (*height);

    printf("Image Resolution: %d x %d\n", *width, *height);
    printf("Total Pixels to process: %ld\n", total_pixels);

    Pixel *img = (Pixel *)malloc(total_pixels * sizeof(Pixel));
    if (!img) { exit(1); }
    int row_stride = (*width) * 3;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    long pixel_count = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        for (int x = 0; x < *width; x++) {
            img[pixel_count].r = buffer[0][x * 3];
            img[pixel_count].g = buffer[0][x * 3 + 1];
            img[pixel_count].b = buffer[0][x * 3 + 2];
            img[pixel_count].cluster_id = -1;
            pixel_count++;
        }
    }
    printf("Finished! Total pixels processed: %ld\n", pixel_count);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return img;
}



void write_jpeg_file(const char *filename, Pixel *img, int width, int height, int quality){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile;
    JSAMPROW row_pointer[1];
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    if ((outfile = fopen(filename, "wb")) == NULL){
        fprintf(stderr, "Error: Can't create %s\n", filename);
        exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * 3;
    unsigned char *raw_row = (unsigned char *)malloc(row_stride);

    int pixel_index = 0;
    while (cinfo.next_scanline < cinfo.image_height){
        for (int x = 0; x < width; x++){
            raw_row[x * 3]     = img[pixel_index].r;
            raw_row[x * 3 + 1] = img[pixel_index].g;
            raw_row[x * 3 + 2] = img[pixel_index].b;
            pixel_index++;
        }
        row_pointer[0] = raw_row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    free(raw_row);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);
}

// --- K-Means Logic ---
//calloc() gives you a zero-initialized buffer, while malloc() leaves the memory uninitialized.
void kMeansSegmentation(Pixel *img, int num_pixels, int k, int max_iterations){
    Centroid *centroids = (Centroid *)malloc(k * sizeof(Centroid));
    unsigned int total_pixels = 0;
    unsigned int dr, dg, db, dist;
    // 1. Random Initialization
    srand(ID); // Fixed seed for reproducibility
    for (int i = 0; i < k; i++){// For each centroid = k
        unsigned int idx = rand() % num_pixels; // Random pixel index
        centroids[i].r = img[idx].r;
        centroids[i].g = img[idx].g;
        centroids[i].b = img[idx].b;
    }

    // 2. Iteration Loop
    for (int iter = 0; iter < max_iterations; iter++){
        unsigned int changed = 0;

        // Reset accumulators
        for (int j = 0; j < k; j++) centroids[j].pixel_count = 0;
        unsigned int *sumR = (unsigned int *)calloc(k, sizeof(unsigned int));
        unsigned int *sumG = (unsigned int *)calloc(k, sizeof(unsigned int));
        unsigned int *sumB = (unsigned int *)calloc(k, sizeof(unsigned int));

        // Assign pixels to nearest centroid
        for (int i = 0; i < num_pixels; i++){ // For each pixel
            unsigned int minDist = UINT_MAX;
            unsigned int bestCluster = 0;
            for (int j = 0; j < k; j++){ // For each centroid
                dr = img[i].r - centroids[j].r;
                dg = img[i].g - centroids[j].g;
                db = img[i].b - centroids[j].b;
                dist = dr*dr + dg*dg + db*db;
                if (dist < minDist){// Find the closest centroid
                    minDist = dist;
                    bestCluster = j;
                }
            }

            if (img[i].cluster_id != bestCluster){ // Cluster assignment changed
                img[i].cluster_id = bestCluster;
                changed++;
            }
            // Accumulate sums for centroid update
            sumR[bestCluster] += img[i].r;
            sumG[bestCluster] += img[i].g;
            sumB[bestCluster] += img[i].b;
            centroids[bestCluster].pixel_count++;
        }

        // Update centroids
        for (int j = 0; j < k; j++){// For each centroid
            if (centroids[j].pixel_count > 0){
                centroids[j].r = sumR[j] / centroids[j].pixel_count;
                centroids[j].g = sumG[j] / centroids[j].pixel_count;
                centroids[j].b = sumB[j] / centroids[j].pixel_count;
            }
        }

        free(sumR); free(sumG); free(sumB);
        // Comment this line for Final Version
        //printf("Iteration %d: %d pixels changed clusters\n", iter + 1, changed);
        if (changed == 0) break;
    }

    // 3. Final Recolor (Using Distinct Basic Colors)
    for (int i = 0; i < num_pixels; i++){
        unsigned int id = img[i].cluster_id;
        // Use the helper function to set distinct colors
        getBasicColor(id, &img[i].r, &img[i].g, &img[i].b);
    }
    printf("****************************************\n");
    for(int j = 0; j < k; j++){
        total_pixels+= centroids[j].pixel_count;
        printf("*\tCluster %d.%d: %d pixels\t*\n", j,k,centroids[j].pixel_count);
    }
    printf("**\tTotal pixels assigned: %d\t**\n", total_pixels);
    free(centroids);
}

// --- Main ---
int main(int argc, char *argv[]){
    if (argc != 4){
        printf("Usage: %s <input.jpg> <output.jpg> <k>\n", argv[0]);
        return 1;
    }

    int width, height;
    printf("Reading JPEG image...\n");
    Pixel *img = read_jpeg_file(argv[1], &width, &height);
    int k = atoi(argv[3]);
    printf("Running K-means with K=%d Clusters...\n", k);
    kMeansSegmentation(img, width * height, k, MAX_ITERATIONS);
    write_jpeg_file(argv[2], img, width, height, 90);
    free(img);
    return 0;
}