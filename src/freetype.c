#include "ft.h"

#include <stdio.h>

#ifdef __WIN32

int ft_main(int argc, char *argv[])
{
  return 1;
}

#else // __linux

#include <ft2build.h>
#include FT_FREETYPE_H

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS (0)
#endif // EXIT_SUCCESS

#ifndef EXIT_FAILURE
#define EXIT_FAILURE (1)
#endif // EXIT_FAILURE

#ifndef NULL
#define NULL ((void *)(0))
#endif // NULL

int ft_main(int argc, char *argv[])
{
  FT_Library library = {0};
  FT_Face face = {0};
  FT_Error error = FT_Init_FreeType(&library);
  if (error != FT_Err_Ok)
  {
    fprintf(stderr, "Error 0x%x\n", error);
    return EXIT_FAILURE;
  }

  error =
    FT_New_Face(library, "/home/janne/git/test/shared/fonts/NotoSans-Regular.ttf", 0,
                &face);
  if (error == FT_Err_Unknown_File_Format)
  {
    fprintf(stderr, "Unknown file format!\n");
    return EXIT_FAILURE;
  }
  else if (error != FT_Err_Ok)
  {
    fprintf(stderr, "Error 0x%x\n", error);
    return EXIT_FAILURE;
  }

  error = FT_Set_Pixel_Sizes(face, 0, 24);
  if (error != FT_Err_Ok)
  {
    fprintf(stderr, "Error setting char size: 0x%x\n", error);
    return EXIT_FAILURE;
  }

  error = FT_Load_Glyph(face, FT_Get_Char_Index(face, 'g'), FT_LOAD_DEFAULT);
  if (error != FT_Err_Ok)
  {
    fprintf(stderr, "Error loading glyph: 0x%x\n", error);
    return EXIT_FAILURE;
  }

  if (FT_HAS_VERTICAL(face))
    printf("Metrics:\n\
  width:       %ld\n\
  height:      %ld\n\
  h-bearing-x: %ld\n\
  h-bearing-y: %ld\n\
  h-advance:   %ld\n\
  v-bearing-x: %ld\n\
  v-bearing-y: %ld\n\
  v-advance:   %ld\n",
           face->glyph->metrics.width,
           face->glyph->metrics.height,
           face->glyph->metrics.horiBearingX,
           face->glyph->metrics.horiBearingY,
           face->glyph->metrics.horiAdvance,
           face->glyph->metrics.vertBearingX,
           face->glyph->metrics.vertBearingY,
           face->glyph->metrics.vertAdvance);
  else
    printf("Metrics:\n\
  width:       %ld\n\
  height:      %ld\n\
  h-bearing-x: %ld\n\
  h-bearing-y: %ld\n\
  h-advance:   %ld\n",
           face->glyph->metrics.width,
           face->glyph->metrics.height,
           face->glyph->metrics.horiBearingX,
           face->glyph->metrics.horiBearingY,
           face->glyph->metrics.horiAdvance);


  return EXIT_SUCCESS;
}

#endif // __linux
