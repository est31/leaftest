// this is two times as fast when compiled with -Ofast

// for not tileable textures

// see https://graphics.ethz.ch/~cengizo/Files/Sig15PerceptualDownscaling.pdf

#include <stdlib.h> // malloc, EXIT_*
#include <string.h> // memset
#include <math.h>
#include <png.h>

#define SQR_NP 2 // squareroot of the patch size, recommended: 2


#define EXIT_PNG(F) if (!F) { \
	fprintf(stderr, "%s\n", bild.message); \
	return EXIT_FAILURE; \
}

#define CLAMP(V, A, B) (V) < (A) ? (A) : (V) > (B) ? (B) : (V)

#define u8 unsigned char

struct pixel {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};
#define PIXELBYTES 4

struct matrix {
	int w;
	int h;
	float *data;
};

struct image {
	int w;
	int h;
	struct pixel *pixels;
};

/*! \brief get y, cb and cr values each in [0;1] from u8 r, g and b values
 *
 * there's gamma correction,
 * see http://www.ericbrasseur.org/gamma.html?i=1#Assume_a_gamma_of_2.2
 * 0.5 is added to cb and cr to have them in [0;1]
 */
static void rgb2ycbcr(u8 or, u8 og, u8 ob, float *y, float *cb, float *cr)
{
	float divider = 1.0f / 255.0f;
	float r = powf(or * divider, 2.2f);
	float g = powf(og * divider, 2.2f);
	float b = powf(ob * divider, 2.2f);
	*y = (0.299f * r + 0.587f * g + 0.114f * b);
	*cb = (-0.168736f * r - 0.331264f * g + 0.5f * b) + 0.5f;
	*cr = (0.5f * r - 0.418688f * g - 0.081312f * b) + 0.5f;
}

/*! \brief the inverse of the function above
 *
 * numbers from http://www.equasys.de/colorconversion.html
 * if values are too big or small, they're clamped
 */
static void ycbcr2rgb(float y, float cb, float cr, u8 *r, u8 *g, u8 *b)
{
	float vr = (y + 1.402f * (cr - 0.5f));
	float vg = (y - 0.344136f * (cb - 0.5f) - 0.714136f * (cr - 0.5f));
	float vb = (y + 1.772f * (cb - 0.5f));
	float exponent = 1.0f / 2.2f;
	vr = powf(vr, exponent);
	vg = powf(vg, exponent);
	vb = powf(vb, exponent);
	*r = CLAMP(vr * 255.0f, 0, 255);
	*g = CLAMP(vg * 255.0f, 0, 255);
	*b = CLAMP(vb * 255.0f, 0, 255);
}

/*! \brief Convert an rgba image to 4 ycbcr matrices with values in [0, 1]
 */
static struct matrix *image_to_matrices(struct image *bild)
{
	int w = bild->w;
	int h = bild->h;
	struct matrix *matrices = malloc(
		PIXELBYTES * sizeof(struct matrix));
	for (int i = 0; i < PIXELBYTES; ++i) {
		matrices[i].w = w;
		matrices[i].h = h;
		matrices[i].data = malloc(w * h * sizeof(float));
	}
	for (int i = 0; i < w * h; ++i) {
		struct pixel px = bild->pixels[i];
		// put y, cb, cr and transpatency into the matrices
		rgb2ycbcr(px.r, px.g, px.b,
			&matrices[0].data[i], &matrices[1].data[i], &matrices[2].data[i]);
		float divider = 1.0f / 255.0f;
		matrices[3].data[i] = px.a * divider;
	}
	return matrices;
}

/*! \brief Convert 4 matrices to an rgba image
 *
 * Note that matrices becomes freed.
 */
static struct image *matrices_to_image(struct matrix *matrices)
{
	struct image *bild = malloc(sizeof(struct image));
	int w = matrices[0].w;
	int h = matrices[0].h;
	bild->w = w;
	bild->h = h;
	struct pixel *pixels = malloc(w * h * PIXELBYTES);
	for (int i = 0; i < w * h; ++i) {
		struct pixel *px = &pixels[i];
		ycbcr2rgb(matrices[0].data[i], matrices[1].data[i], matrices[2].data[i],
			&px->r, &px->g, &px->b);
		float a = matrices[3].data[i] * 255;
		px->a = CLAMP(a, 0, 255);
	}
	for (int i = 0; i < PIXELBYTES; ++i) {
		free(matrices[i].data);
	}
	free(matrices);
	bild->pixels = pixels;
	return bild;
}

/*! \brief The actual downscaling algorithm
 *
 * \param mat The 4 matrices obtained form image_to_matrices.
 * \param s The factor by which the image should become downscaled.
 */
static void downscale_perc(struct matrix *mat, int s)
{
	// preparation
	int w = mat->w; // input width
	int h = mat->h;
	float *input = mat->data;
	int w2 = w / s; // output width
	int h2 = h / s;
	int input_size = w * h * sizeof(float);
	int output_size = input_size / (s * s);
	//~ fprintf(stderr, "w, h, s: %d, %d, %d\n", w,h,s);
	float *l = malloc(output_size);
	float *l2 = malloc(output_size);
	float *d = malloc(output_size);

	// set d's entries to 0 (because it's used for a sum)
	for (int i = 0; i < w2 * h2; ++i)
		d[i] = 0;

	// get l and l2, the input image and it's size are used only here
	for (int ysm = 0; ysm < h2; ++ysm) {
		for (int xsm = 0; xsm < w2; ++xsm) {
			// xsm and ysm are coords for the subsampled image
			int x = xsm * s;
			int y = ysm * s;
			float acc = 0;
			float acc2 = 0;
			for (int yc = y; yc < y + s; ++yc) {
				for (int xc = x; xc < x + s; ++xc) {
					// if xc or yc is out of the image, set it to the border
					int y_cl = CLAMP(yc, 0, h-1);
					int x_cl = CLAMP(xc, 0, w-1);
					float v = input[y_cl * w + x_cl];
					acc += v;
					acc2 += v * v;
				}
			}
			int ism = ysm*w2+xsm;
			float divider = 1.0f / (s * s);
			l[ism] = acc * divider;
			l2[ism] = acc2 * divider;
		}
	}

	float patch_sz_div = 1.0f / (SQR_NP * SQR_NP);
	// calculate the average of the results of all possible patch sets
	for (int y_offset = 0; y_offset > -SQR_NP; --y_offset) {
		for (int x_offset = 0; x_offset > -SQR_NP; --x_offset) {
			float *m = malloc(output_size);
			float *r = malloc(output_size);

			// get m
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc = 0;
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							int y_cl = CLAMP(yc, 0, h2-1);
							int x_cl = CLAMP(xc, 0, w2-1);
							int i = y_cl * w2 + x_cl;
							acc += l[i];
						}
					}
					m[y*w2+x] = acc * patch_sz_div;
				}
			}

			// get r
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc = 0;
					float acc2 = 0;
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							int y_cl = CLAMP(yc, 0, h2-1);
							int x_cl = CLAMP(xc, 0, w2-1);
							int i = y_cl * w2 + x_cl;
							acc += l[i] * l[i];
							acc2 += l2[i];
						}
					}
					int i = y*w2+x;
					float mv = m[i];
					float slv = acc * patch_sz_div - mv * mv;
					float shv = acc2 * patch_sz_div - mv * mv;
					if (slv >= 0.000001f) // epsilon is 10⁻⁶
						r[i] = sqrtf(shv / slv);
					else
						r[i] = 0;
				}
			}

			// get d, which is the output
			for (int y = 0; y < h2; ++y) {
				for (int x = 0; x < w2; ++x) {
					float acc_m = 0;
					float acc_r = 0;
					float acc_t = 0;
					int ys = y - (y + SQR_NP + y_offset) % SQR_NP;
					int xs = x - (x + SQR_NP + x_offset) % SQR_NP;
					for (int yc = ys; yc < ys + SQR_NP; ++yc) {
						for (int xc = xs; xc < xs + SQR_NP; ++xc) {
							int y_cl = CLAMP(yc, 0, h2-1);
							int x_cl = CLAMP(xc, 0, w2-1);
							int i = y_cl * w2 + x_cl;
							acc_m += m[i];
							acc_r += r[i];
							acc_t += r[i] * m[i];
						}
					}
					int i = y*w2+x;
					d[i] += (
							acc_m * patch_sz_div
							+ acc_r * patch_sz_div * l[i]
							- acc_t * patch_sz_div
						);
				}
			}
			free(m);
			free(r);
		}
	}

	// divide values in d for the (arithmetic) average
	for (int i = 0; i < w2 * h2; ++i)
		d[i] *= patch_sz_div;


	// update the matrix
	mat->data = d;
	mat->w = w2;
	mat->h = h2;

	// tidy up
	free(input);
	free(l);
	free(l2);
}

/*! \brief Function which calls functions for downscaling
 *
 * \param bild The image, it's content is changed when finished.
 * \param downscale_factor Must be a natural number.
 */
void downscale_an_image(struct image **bild, int downscale_factor)
{
	struct matrix *matrices = image_to_matrices(*bild);
	for (int i = 0; i < PIXELBYTES; ++i) {
		downscale_perc(&(matrices[i]), downscale_factor);
	}
	*bild = matrices_to_image(matrices);
}

int main(int argc, char **args)
{
	if (argc != 2) {
		fprintf(stderr, "Missing arguments, usage: ./perc <downscaling_factor>"
			"\n");
		return EXIT_FAILURE;
	}
	int downscaling_factor = atoi(args[1]);
	if (downscaling_factor < 2) {
		fprintf(stderr, "Invalid downscaling factor: %d\n",
			downscaling_factor);
		return EXIT_FAILURE;
	}

	png_image bild;
	memset(&bild, 0, sizeof(bild));
	bild.version = PNG_IMAGE_VERSION;
	EXIT_PNG(png_image_begin_read_from_stdio(&bild, stdin))

	int w = bild.width;
	int h = bild.height;
	bild.flags = PNG_IMAGE_FLAG_COLORSPACE_NOT_sRGB;
	bild.format = PNG_FORMAT_RGBA;
	struct pixel *pixels = malloc(w * h * 4);
	EXIT_PNG(png_image_finish_read(&bild, NULL, pixels, 0, NULL))


	struct image origpic = {w = w, h = h, pixels = pixels};
	struct image *newpic = &origpic;
	downscale_an_image(&newpic, downscaling_factor);
	bild.width = newpic->w;
	bild.height = newpic->h;
	free(pixels);
	pixels = newpic->pixels;
	free(newpic);


	EXIT_PNG(png_image_write_to_stdio(&bild, stdout, 0, pixels, 0, NULL));
	free(pixels); // redundant free to feed valgrind
	return EXIT_SUCCESS;
}
