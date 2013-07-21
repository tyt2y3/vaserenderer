#include <stdio.h>
#include <png.h>

int	// 0 on failure, 1 on success
save_png(const char *filename,	// filename
	unsigned char* buffer,		// buffer of an image
	int w,		// width of image
	int h,		// height
	int b)		// color depth/ bytes per pixel
{
	int	y;						// Current row
	const unsigned char	*ptr;	// Pointer to image data
	FILE				*fp;	// File pointer
	png_structp			pp;		// PNG data
	png_infop			info;	// PNG image info

	if ( !filename)
	{
		printf("invalid filename.\n");
		return 0;
	}
	if ( !buffer)
	{
		printf("null pointer buffer.\n");
		return 0;
	}
	
	// Create the output file...
	if ((fp = fopen(filename, "wb")) == NULL)
	{
		printf("Unable to create PNG image.\n");
		return (0);
	}

	// Create the PNG image structures...
	pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!pp)
	{
		fclose(fp);
		printf("Unable to create PNG data.\n");
		return (0);
	}

	info = png_create_info_struct(pp);
	if (!info)
	{
		fclose(fp);
		png_destroy_write_struct(&pp, 0);
		printf("Unable to create PNG image information.\n");
		return (0);
	}

	if (setjmp(png_jmpbuf(pp)))
	{
		fclose(fp);
		png_destroy_write_struct(&pp, &info);
		printf("Unable to write PNG image.\n");
		return (0);
	}

	png_init_io(pp, fp);

	png_set_compression_level(pp, Z_BEST_COMPRESSION);
	png_set_IHDR(pp, info, w, h, 8,
	       b==3? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);
	png_set_sRGB(pp, info, PNG_sRGB_INTENT_PERCEPTUAL);
	png_set_sRGB_gAMA_and_cHRM(pp, info, PNG_INFO_sRGB);
	
	png_write_info(pp, info);
	
	ptr = buffer;
	
	for (y = 0; y < h; y ++)
	{
		png_write_row(pp, (png_byte *)ptr);
		ptr += w*b;
	}

	png_write_end(pp, info);
	png_destroy_write_struct(&pp, 0);

	fclose(fp);
	return (1);
}


unsigned char*	// 0 on failure, address of memory buffer on success
				// must *delete[]* the buffer manually after use
read_png( const char* filename, // filename
	int* width,			// store image width into pointer
	int* height,		// store image height into pointer
	int* channels)		// number of channels
{
	int		i;			// Looping var
	FILE		*fp;	// File pointer
	png_structp	pp;		// PNG read pointer
	png_infop	info;	// PNG info pointers
	png_bytep	*rows;	// PNG row pointers

	// Open the PNG file...
	if ((fp = fopen(filename, "rb")) == NULL)
	{
		printf( "Unable to open file %s.\n", filename);
		return 0;
	}

	// Setup the PNG data structures...
	pp   = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info = png_create_info_struct(pp);

	if (setjmp(pp->jmpbuf))
	{
		printf("Unable to read. png file %s contains errors.\n", filename);
		return 0;
	}

	// Initialize the PNG read "engine"...
	png_init_io(pp, fp);

	// Get the image dimensions and convert to grayscale or RGB...
	png_read_info(pp, info);

	if (info->color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_expand(pp);

	if (info->color_type & PNG_COLOR_MASK_COLOR)
	*channels = 3;
	else
	*channels = 1;

	if ((info->color_type & PNG_COLOR_MASK_ALPHA) || info->num_trans)
	(*channels) ++;

	if ( *channels != 3 && *channels != 4)
	{
		printf("png image other than 3 or 4 channels is not supported.\n");
		return 0;
	}

	*width = info->width;
	*height = info->height;

	if (info->bit_depth < 8)
	{
	png_set_packing(pp);
	png_set_expand(pp);
	}
	else if (info->bit_depth == 16)
	png_set_strip_16(pp);

	// Handle transparency...
	if (png_get_valid(pp, info, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha(pp);

	unsigned char* array = new unsigned char[(*width) * (*height) * (*channels)];

	// Allocate pointers...
	rows = new png_bytep[(*height)];

	for (i = 0; i < (*height); i ++)
	rows[i] = (png_bytep)(array + i * (*width) * (*channels));

	// Read the image, handling interlacing as needed...
	for (i = png_set_interlace_handling(pp); i > 0; i --)
	png_read_rows(pp, rows, NULL, (*height));

	#ifdef WIN32
	// Some Windows graphics drivers don't honor transparency when RGB == white
	if (*channels == 4) {
	// Convert RGB to 0 when alpha == 0...
	unsigned char* ptr = (unsigned char*) array;
	for (i = (*width) * (*height); i > 0; i --, ptr += 4)
	if (!ptr[3]) ptr[0] = ptr[1] = ptr[2] = 0;
	}
	#endif //WIN32

	// Free memory
	delete[] rows;

	png_read_end(pp, info);
	png_destroy_read_struct(&pp, &info, NULL);

	fclose(fp);

	return array;
}
