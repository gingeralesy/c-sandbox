#include "ft.h"

#ifdef __WIN32

int ft_main(int argc, char *argv[])
{
  return 1;
}

#else // __linux

#include <ft2build.h>
#include FT_FREETYPE_H


int ft_main(int argc, char *argv[])
{
  static const char *DEFAULT_FONT = "/home/janne/.fonts/NotoSans-Regular.ttf";
  const char *fontfile = (1 < argc ? argv[1] : DEFAULT_FONT);
  FT_Library library = {0};
  FT_Face face = {0};
  FT_Error error = FT_Init_FreeType(&library);
  if (error != FT_Err_Ok)
  {
    fprintf(stderr, "Initialisation error: 0x%x\n", error);
    return EXIT_FAILURE;
  }

  error = FT_New_Face(library, fontfile, 0, &face);
  if (error != FT_Err_Ok)
  {
    switch (error)
    {
    case FT_Err_Cannot_Open_Resource:
      fprintf(stderr, "No such font: %s\n", fontfile);
      break;
    case FT_Err_Unknown_File_Format:
      fprintf(stderr, "Unknown file format!\n");
      break;
    default:
      fprintf(stderr, "Font load error: 0x%x\n", error);
      break;
    }
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
    fprintf(stderr,
            "Metrics:\n"
            "  width:       %ld\n"
            "  height:      %ld\n"
            "  h-bearing-x: %ld\n"
            "  h-bearing-y: %ld\n"
            "  h-advance:   %ld\n"
            "  v-bearing-x: %ld\n"
            "  v-bearing-y: %ld\n"
            "  v-advance:   %ld\n",
            face->glyph->metrics.width,
            face->glyph->metrics.height,
            face->glyph->metrics.horiBearingX,
            face->glyph->metrics.horiBearingY,
            face->glyph->metrics.horiAdvance,
            face->glyph->metrics.vertBearingX,
            face->glyph->metrics.vertBearingY,
            face->glyph->metrics.vertAdvance);
  else
    fprintf(stderr,
            "Metrics:\n"
            "  width:       %ld\n"
            "  height:      %ld\n"
            "  h-bearing-x: %ld\n"
            "  h-bearing-y: %ld\n"
            "  h-advance:   %ld\n",
            face->glyph->metrics.width,
            face->glyph->metrics.height,
            face->glyph->metrics.horiBearingX,
            face->glyph->metrics.horiBearingY,
            face->glyph->metrics.horiAdvance);
  fflush(stderr);

  return EXIT_SUCCESS;
}

#endif // __linux
