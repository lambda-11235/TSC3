/* TSC is a two-dimensional jump’n’run platform game.
 * Copyright © 2017 The TSC Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#include <png.h>
#include "commandline.hpp"

using namespace std;

/* How many bytes libpng should check for whether we're dealing
 * with a PNG file. */
static const size_t PNG_MAGIC_BYTE_COUNT = 8;

/* Bounding box. If a tile in a tileset has
 * no bounding box, the `x' and `y' members are set to -1. */
struct bbox {
    int x;
    int y;
    int w;
    int h;
};

bool check_if_png(FILE* fp)
{
    unsigned char buf[PNG_MAGIC_BYTE_COUNT];

    if (fread(buf, 1, PNG_MAGIC_BYTE_COUNT, fp) != PNG_MAGIC_BYTE_COUNT)
        return false;

    return !png_sig_cmp(buf, 0, PNG_MAGIC_BYTE_COUNT);
}

/* Checks whether a pixel is painted. The assumptions is that anything
 * where the user drew the bounding box is not fully transparent, i.e.
 * only alpha is checked. Checking the RGB values would be difficult,
 * because different image programmes can set different values for
 * fully-transparent pixels. The GIMP sets RGBA=(0|0|0|0), i.e.
 * fully transparent black, but other programmes may as well just
 * do RGBA=(255|255|255|0) or however they see fit. Thus, the alpha
 * value is the only one one can realy on. */
bool is_painted_pixel(png_byte* pix)
{
    /* Third component is alpha value;
     * 0=transparent, 255=opaque as per PNG format spec. */
    return pix[3] > 0;
}

vector<bbox> read_bboxes(int htiles, int vtiles, int imgwidth, int imgheight, png_bytep row_pointers[])
{
    int tilewidth  = imgwidth  / htiles;
    int tileheight = imgheight / vtiles;

    vector<bbox> bboxes(htiles * vtiles);

    // Tile by tile, determine the bboxes.
    for(int tileno=0; tileno < htiles * vtiles; tileno++) {
        int vstart = tileno / htiles * tileheight;
        int vend   = vstart + tileheight; // excluded, this is one behind the last row

        // Get element to work with
        struct bbox& box = bboxes[tileno];
        box.x = box.y = box.w = box.h = -1;

        for(int y=vstart; y < vend; y++) {
            png_bytep row = row_pointers[y];

            if (box.x >= 0 && box.w >= 0) {
                /* 3. Determine the height of the box. The bottom-left corner
                 * is used for that, and that one's reached either when the loop
                 * enters white pixels again or the next row is beyond the tile's
                 * edge. */
                png_bytep p_pixel = &(row[box.x*4]);

                if (!is_painted_pixel(p_pixel) || y+1 >= vend) {
                    box.h = y - box.y;
                    break; // Skip rest of tile iteration, tile is complete.
                }

                continue; // Skip row iteration, not needed anymore
            }

            int hstart = tileno % htiles * tilewidth; // included, this is the first column
            int hend   = hstart + tilewidth; // excluded, this is one behind the last column
            for(int x=hstart; x < hend; x++) {
                png_bytep p_pixel = &(row[x*4]);

                if (box.x < 0) {
                    // 1. Determine the top-left corner of the bbox.
                    if (is_painted_pixel(p_pixel)) {
                        box.x = x;
                        box.y = y;
                    }
                }
                else if (box.w < 0) {
                    /* 2. Determine the width of the bbox. The top-right corner
                     * is used for that, and that one's reached either when the
                     * loop enters white pixels again or the next column is beyond
                     * the tile's edge. */
                    if (!is_painted_pixel(p_pixel) || x+1 >= hend) {
                        box.w = x - box.x;
                    }
                }
            }
        }
    }

    return bboxes;
}

vector<bbox> extract_bboxes(FILE* infile, int htiles, int vtiles)
{
    png_structp p_png    = NULL;
    png_infop p_png_info = NULL;
    vector<bbox> bboxes;

    p_png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                   NULL, NULL, NULL);
    if (!p_png) {
        cerr << "Internal error: cannot create PNG struct" << endl;
        return bboxes;
    }

    if (!(p_png_info = png_create_info_struct(p_png))) {
        png_destroy_read_struct(&p_png, NULL, NULL);
        cerr << "Internal error: cannot create PNG info struct" << endl;
        return bboxes;
    }

    /* If libpng wants to signal an error, it will longjmp() here
     * (see libpng manual). */
    if (setjmp(png_jmpbuf(p_png))) {
        png_destroy_read_struct(&p_png, &p_png_info, NULL);
        cerr << "Internal error while reading the PNG file" << endl;
        return bboxes;
    }

    // Read PNG header
    png_init_io(p_png, infile);
    png_set_sig_bytes(p_png, PNG_MAGIC_BYTE_COUNT); // Check already done prior
    png_read_info(p_png, p_png_info);

    // Get some info
    int width  = png_get_image_width(p_png, p_png_info);
    int height = png_get_image_height(p_png, p_png_info);

    png_set_interlace_handling(p_png);
    png_read_update_info(p_png, p_png_info);

    // Read actual image into memory
    png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int y=0; y < height; y++)
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(p_png, p_png_info));
    png_read_image(p_png, row_pointers);

    // Read bounding boxes
    bboxes = move(read_bboxes(htiles, vtiles, width, height, row_pointers));

    // Cleanup
    png_destroy_read_struct(&p_png, &p_png_info, NULL);
    for(int y=0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    return bboxes;
}

void output_xml(FILE* infile, int htiles, int vtiles)
{
    if (htiles <= 0 || vtiles <= 0) {
        cerr << "Error: Rows and/or columns specification required. Did you pass -t?" << endl;
        return;
    }

    vector<bbox> bboxes = extract_bboxes(infile, htiles, vtiles);

    if (bboxes.empty())
        cerr << "Warning: No collision rectangles found" << endl;

    if (cmdline.authors.empty())
        cerr << "Warning: No authors given. Pass at least one -a option." << endl;

    // Output goes to real file if passed, otherwise standard output.
    ofstream outfilefile;
    if (!cmdline.outfile.empty())
        outfilefile.open(cmdline.outfile);
    ostream& outfile = cmdline.outfile.empty() ? cout : outfilefile;

    outfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
            << "<tileset version=\"1.0\">" << endl
            << "  <cols>" << cmdline.htiles << "</cols>" << endl
            << "  <rows>" << cmdline.vtiles << "</rows>" << endl
            << "  <authors>" << endl;

    for(auto iter=cmdline.authors.begin(); iter != cmdline.authors.end(); iter++) {
        outfile << "    <author>" << endl
                << "      <name>" << iter->first << "</name>" << endl
                << "      <detail>" << iter->second << "</detail>" << endl
                << "    </author>" << endl;
    }

    outfile << "  </authors>" << endl
            << "  <tiles>" << endl;

    for(const bbox& box: bboxes) {
        outfile << "    <tile>" << endl
                << "      <colrect x=\""
                  << box.x
                  << "\" y=\""
                  << box.y
                  << "\" width=\""
                  << box.w
                  << "\" height=\""
                  << box.h
                  << "\"/>"
                  << endl
                << "    </tile>" << endl;
    }

    outfile << "  </tiles>" << endl
            << "</tileset>" << endl;
}

void input_xml(FILE* input)
{
    /* TODO */
}

int main(int argc, char* argv[])
{
    parse_commandline(argc, argv);

    FILE* infile = cmdline.infile.empty() ? stdin : fopen(cmdline.infile.c_str(), "rb");
    if (!infile) {
        cerr << "Failed to open input file." << endl;
        return 1;
    }

    if (check_if_png(infile))
        output_xml(infile, cmdline.htiles, cmdline.vtiles);
    else
        input_xml(infile);

    fclose(infile);

    return 0;
}