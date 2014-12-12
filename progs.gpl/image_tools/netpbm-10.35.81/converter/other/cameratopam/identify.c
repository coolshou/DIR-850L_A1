#define _BSD_SOURCE   /* Make sure strcasecmp() is in string.h */
#include <string.h>

#include "pm.h"

#include "global_variables.h"
#include "util.h"
#include "foveon.h"
#include "canon.h"
#include "dng.h"
#include "ljpeg.h"
#include "camera.h"

#include "identify.h"


#if HAVE_INT64
   static bool const have64BitArithmetic = true;
#else
   static bool const have64BitArithmetic = false;
#endif


static loadRawFn load_raw;

/* This does the same as the function of the same name in the GNU C library */
static const char *memmem_internal (const char *haystack, size_t haystacklen,
                     const char *needle, size_t needlelen)
{
  const char *c;
  for (c = haystack; c <= haystack + haystacklen - needlelen; c++)
    if (!memcmp (c, needle, needlelen))
      return c;
  return NULL;
}

/*
   Thanks to Adobe for providing these excellent CAM -> XYZ matrices!
 */
static void 
adobe_coeff()
{
  static const struct {
    const char *prefix;
    short trans[12];
  } table[] = {
    { "Canon EOS D2000C",
    { 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
    { "Canon EOS D30",
    { 9805,-2689,-1312,-5803,13064,3068,-2438,3075,8775 } },
    { "Canon EOS D60",
    { 6188,-1341,-890,-7168,14489,2937,-2640,3228,8483 } },
    { "Canon EOS 10D",
    { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon EOS 20D",
    { 6599,-537,-891,-8071,15783,2424,-1983,2234,7462 } },
    { "Canon EOS-1Ds Mark II",
    { 6517,-602,-867,-8180,15926,2378,-1618,1771,7633 } },
    { "Canon EOS-1D Mark II",
    { 6264,-582,-724,-8312,15948,2504,-1744,1919,8664 } },
    { "Canon EOS-1DS",
    { 4374,3631,-1743,-7520,15212,2472,-2892,3632,8161 } },
    { "Canon EOS-1D",
    { 6906,-278,-1017,-6649,15074,1621,-2848,3897,7611 } },
    { "Canon EOS",
    { 8197,-2000,-1118,-6714,14335,2592,-2536,3178,8266 } },
    { "Canon PowerShot 600",
    { -3822,10019,1311,4085,-157,3386,-5341,10829,4812,-1969,10969,1126 } },
    { "Canon PowerShot A50",
    { -5300,9846,1776,3436,684,3939,-5540,9879,6200,-1404,11175,217 } },
    { "Canon PowerShot A5",
    { -4801,9475,1952,2926,1611,4094,-5259,10164,5947,-1554,10883,547 } },
    { "Canon PowerShot G1",
    { -4778,9467,2172,4743,-1141,4344,-5146,9908,6077,-1566,11051,557 } },
    { "Canon PowerShot G2",
    { 9087,-2693,-1049,-6715,14382,2537,-2291,2819,7790 } },
    { "Canon PowerShot G3",
    { 9212,-2781,-1073,-6573,14189,2605,-2300,2844,7664 } },
    { "Canon PowerShot G5",
    { 9757,-2872,-933,-5972,13861,2301,-1622,2328,7212 } },
    { "Canon PowerShot G6",
    { 9877,-3775,-871,-7613,14807,3072,-1448,1305,7485 } },
    { "Canon PowerShot Pro1",
    { 10062,-3522,-999,-7643,15117,2730,-765,817,7323 } },
    { "Canon PowerShot Pro70",
    { -4155,9818,1529,3939,-25,4522,-5521,9870,6610,-2238,10873,1342 } },
    { "Canon PowerShot Pro90",
    { -4963,9896,2235,4642,-987,4294,-5162,10011,5859,-1770,11230,577 } },
    { "Canon PowerShot S30",
    { 10566,-3652,-1129,-6552,14662,2006,-2197,2581,7670 } },
    { "Canon PowerShot S40",
    { 8510,-2487,-940,-6869,14231,2900,-2318,2829,9013 } },
    { "Canon PowerShot S45",
    { 8163,-2333,-955,-6682,14174,2751,-2077,2597,8041 } },
    { "Canon PowerShot S50",
    { 8882,-2571,-863,-6348,14234,2288,-1516,2172,6569 } },
    { "Canon PowerShot S70",
    { 9976,-3810,-832,-7115,14463,2906,-901,989,7889 } },
    { "Contax N Digital",
    { 7777,1285,-1053,-9280,16543,2916,-3677,5679,7060 } },
    { "EPSON R-D1",
    { 6827,-1878,-732,-8429,16012,2564,-704,592,7145 } },
    { "FUJIFILM FinePix E550",
    { 11044,-3888,-1120,-7248,15168,2208,-1531,2277,8069 } },
    { "FUJIFILM FinePix F700",
    { 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
    { "FUJIFILM FinePix S20Pro",
    { 10004,-3219,-1201,-7036,15047,2107,-1863,2565,7736 } },
    { "FUJIFILM FinePix S2Pro",
    { 12492,-4690,-1402,-7033,15423,1647,-1507,2111,7697 } },
    { "FUJIFILM FinePix S3Pro",
    { 11807,-4612,-1294,-8927,16968,1988,-2120,2741,8006 } },
    { "FUJIFILM FinePix S5000",
    { 8754,-2732,-1019,-7204,15069,2276,-1702,2334,6982 } },
    { "FUJIFILM FinePix S5100",
    { 11940,-4431,-1255,-6766,14428,2542,-993,1165,7421 } },
    { "FUJIFILM FinePix S7000",
    { 10190,-3506,-1312,-7153,15051,2238,-2003,2399,7505 } },
    { "Kodak DCS315C",
    { 17523,-4827,-2510,756,8546,-137,6113,1649,2250 } },
    { "Kodak DCS330C",
    { 20620,-7572,-2801,-103,10073,-396,3551,-233,2220 } },
    { "KODAK DCS420",
    { 10868,-1852,-644,-1537,11083,484,2343,628,2216 } },
    { "KODAK DCS460",
    { 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
    { "KODAK EOSDCS1",
    { 10592,-2206,-967,-1944,11685,230,2206,670,1273 } },
    { "KODAK EOSDCS3B",
    { 9898,-2700,-940,-2478,12219,206,1985,634,1031 } },
    { "Kodak DCS520C",
    { 24542,-10860,-3401,-1490,11370,-297,2858,-605,3225 } },
    { "Kodak DCS560C",
    { 20482,-7172,-3125,-1033,10410,-285,2542,226,3136 } },
    { "Kodak DCS620C",
    { 23617,-10175,-3149,-2054,11749,-272,2586,-489,3453 } },
    { "Kodak DCS620X",
    { 13095,-6231,154,12221,-21,-2137,895,4602,2258 } },
    { "Kodak DCS660C",
    { 18244,-6351,-2739,-791,11193,-521,3711,-129,2802 } },
    { "Kodak DCS720X",
    { 11775,-5884,950,9556,1846,-1286,-1019,6221,2728 } },
    { "Kodak DCS760C",
    { 16623,-6309,-1411,-4344,13923,323,2285,274,2926 } },
    { "Kodak DCS Pro SLR",
    { 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
    { "Kodak DCS Pro 14nx",
    { 5494,2393,-232,-6427,13850,2846,-1876,3997,5445 } },
    { "Kodak DCS Pro 14",
    { 7791,3128,-776,-8588,16458,2039,-2455,4006,6198 } },
    { "Kodak ProBack645",
    { 16414,-6060,-1470,-3555,13037,473,2545,122,4948 } },
    { "Kodak ProBack",
    { 21179,-8316,-2918,-915,11019,-165,3477,-180,4210 } },
    { "LEICA DIGILUX 2",
    { 11340,-4069,-1275,-7555,15266,2448,-2960,3426,7685 } },
    { "Leaf Valeo",
    { 8236,1746,-1314,-8251,15953,2428,-3673,5786,5771 } },
    { "Minolta DiMAGE 5",
    { 8983,-2942,-963,-6556,14476,2237,-2426,2887,8014 } },
    { "Minolta DiMAGE 7",
    { 9144,-2777,-998,-6676,14556,2281,-2470,3019,7744 } },
    { "Minolta DiMAGE A1",
    { 9274,-2547,-1167,-8220,16323,1943,-2273,2720,8340 } },
    { "MINOLTA DiMAGE A200",
    { 8560,-2487,-986,-8112,15535,2771,-1209,1324,7743 } },
    { "Minolta DiMAGE A2",
    { 9097,-2726,-1053,-8073,15506,2762,-966,981,7763 } },
    { "MINOLTA DYNAX 7D",
    { 10239,-3104,-1099,-8037,15727,2451,-927,925,6871 } },
    { "NIKON D100",
    { 5915,-949,-778,-7516,15364,2282,-1228,1337,6404 } },
    { "NIKON D1H",
    { 7577,-2166,-926,-7454,15592,1934,-2377,2808,8606 } },
    { "NIKON D1X",
    { 7620,-2173,-966,-7604,15843,1805,-2356,2811,8439 } },
    { "NIKON D1",
    { 7559,-2130,-965,-7611,15713,1972,-2478,3042,8290 } },
    { "NIKON D2H",
    { 5710,-901,-615,-8594,16617,2024,-2975,4120,6830 } },
    { "NIKON D70",
    { 7732,-2422,-789,-8238,15884,2498,-859,783,7330 } },
    { "NIKON E995", /* copied from E5000 */
    { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E2500",
    { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E4500",
    { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E5000",
    { -5547,11762,2189,5814,-558,3342,-4924,9840,5949,688,9083,96 } },
    { "NIKON E5400",
    { 9349,-2987,-1001,-7919,15766,2266,-2098,2680,6839 } },
    { "NIKON E5700",
    { -5368,11478,2368,5537,-113,3148,-4969,10021,5782,778,9028,211 } },
    { "NIKON E8400",
    { 7842,-2320,-992,-8154,15718,2599,-1098,1342,7560 } },
    { "NIKON E8700",
    { 8489,-2583,-1036,-8051,15583,2643,-1307,1407,7354 } },
    { "NIKON E8800",
    { 7971,-2314,-913,-8451,15762,2894,-1442,1520,7610 } },
    { "OLYMPUS C5050",
    { 10508,-3124,-1273,-6079,14294,1901,-1653,2306,6237 } },
    { "OLYMPUS C5060",
    { 10445,-3362,-1307,-7662,15690,2058,-1135,1176,7602 } },
    { "OLYMPUS C70",
    { 10793,-3791,-1146,-7498,15177,2488,-1390,1577,7321 } },
    { "OLYMPUS C80",
    { 8606,-2509,-1014,-8238,15714,2703,-942,979,7760 } },
    { "OLYMPUS E-10",
    { 12745,-4500,-1416,-6062,14542,1580,-1934,2256,6603 } },
    { "OLYMPUS E-1",
    { 11846,-4767,-945,-7027,15878,1089,-2699,4122,8311 } },
    { "OLYMPUS E-20",
    { 13173,-4732,-1499,-5807,14036,1895,-2045,2452,7142 } },
    { "OLYMPUS E-300",
    { 7828,-1761,-348,-5788,14071,1830,-2853,4518,6557 } },
    { "PENTAX *ist D",
    { 9651,-2059,-1189,-8881,16512,2487,-1460,1345,10687 } },
    { "Panasonic DMC-LC1",
    { 11340,-4069,-1275,-7555,15266,2448,-2960,3426,7685 } },
    { "SONY DSC-F828",
    { 7924,-1910,-777,-8226,15459,2998,-1517,2199,6818,-7242,11401,3481 } },
    { "SONY DSC-V3",
    { 9877,-3775,-871,-7613,14807,3072,-1448,1305,7485 } }
  };
  double cc[4][4], cm[4][3], xyz[] = { 1,1,1 };
  char name[130];
  int i, j;

  for (i=0; i < 4; i++)
    for (j=0; j < 4; j++)
      cc[i][j] = i == j;
  sprintf (name, "%s %s", make, model);
  for (i=0; i < sizeof table / sizeof *table; i++)
    if (!strncmp (name, table[i].prefix, strlen(table[i].prefix))) {
      for (j=0; j < 12; j++)
    cm[0][j] = table[i].trans[j];
      dng_coeff (cc, cm, xyz);
      break;
    }
}

/*
   Identify which camera created this file, and set global variables
   accordingly.  Return nonzero if the file cannot be decoded.
 */
int
identify(FILE *       const ifp,
         bool         const use_secondary,
         bool         const use_camera_rgb,
         float        const red_scale,
         float        const blue_scale,
         unsigned int const four_color_rgb,
         const char * const inputFileName,
         loadRawFn *  const loadRawFnP)
{
  char head[32];
  char * c;
  unsigned hlen, fsize, i;
  static const struct {
    int fsize;
    char make[12], model[15], withjpeg;
  } table[] = {
    {    62464, "Kodak",    "DC20"       ,0 },
    {   124928, "Kodak",    "DC20"       ,0 },
    {   311696, "ST Micro", "STV680 VGA" ,0 },  /* SPYz */
    {   787456, "Creative", "PC-CAM 600" ,0 },
    {  2465792, "NIKON",    "E950"       ,1 },  /* or E800,E700 */
    {  2940928, "NIKON",    "E2100"      ,1 },  /* or E2500 */
    {  4771840, "NIKON",    "E990"       ,1 },  /* or E995 */
    {  4775936, "NIKON",    "E3700"      ,1 },  /* or Optio 33WR */
    {  5869568, "NIKON",    "E4300"      ,1 },  /* or DiMAGE Z2 */
    {  5865472, "NIKON",    "E4500"      ,0 },
    {  1976352, "CASIO",    "QV-2000UX"  ,0 },
    {  3217760, "CASIO",    "QV-3*00EX"  ,0 },
    {  6218368, "CASIO",    "QV-5700"    ,0 },
    {  7530816, "CASIO",    "QV-R51"     ,1 },
    {  7684000, "CASIO",    "QV-4000"    ,0 },
    {  7753344, "CASIO",    "EX-Z55"     ,1 },
    {  9313536, "CASIO",    "EX-P600"    ,1 },
    { 10979200, "CASIO",    "EX-P700"    ,1 },
    {  3178560, "PENTAX",   "Optio S"    ,1 },  /*  8-bit */
    {  4841984, "PENTAX",   "Optio S"    ,1 },  /* 12-bit */
    {  6114240, "PENTAX",   "Optio S4"   ,1 },  /* or S4i */
    { 12582980, "Sinar",    ""           ,0 } };
  static const char *corp[] =
    { "Canon", "NIKON", "EPSON", "Kodak", "OLYMPUS", "PENTAX",
      "MINOLTA", "Minolta", "Konica", "CASIO" };
  float tmp;

  /*  What format is this file?  Set make[] if we recognize it. */

  raw_height = raw_width = fuji_width = flip = 0;
  make[0] = model[0] = model2[0] = 0;
  memset (white, 0, sizeof white);
  timestamp = tiff_samples = 0;
  data_offset = meta_length = tiff_data_compression = 0;
  zero_after_ff = is_dng = fuji_secondary = filters = 0;
  black = is_foveon = use_coeff = 0;
  use_gamma = xmag = ymag = 1;
  for (i=0; i < 4; i++) {
    cam_mul[i] = 1 & i;
    pre_mul[i] = 1;
  }
  colors = 3;
  for (i=0; i < 0x1000; i++) curve[i] = i;
  maximum = 0xfff;
#ifdef USE_LCMS
  profile_length = 0;
#endif

  order = get2(ifp);
  hlen = get4(ifp);
  fseek (ifp, 0, SEEK_SET);
  fread (head, 1, 32, ifp);
  fseek (ifp, 0, SEEK_END);
  fsize = ftell(ifp);
  if ((c = (char*)memmem_internal(head, 32, "MMMMRawT", 8))) {
    strcpy (make, "Phase One");
    data_offset = c - head;
    fseek (ifp, data_offset + 8, SEEK_SET);
    fseek (ifp, get4(ifp) + 136, SEEK_CUR);
    raw_width = get4(ifp);
    fseek (ifp, 12, SEEK_CUR);
    raw_height = get4(ifp);
  } else if (order == 0x4949 || order == 0x4d4d) {
    if (!memcmp (head+6, "HEAPCCDR", 8)) {
      data_offset = hlen;
      parse_ciff(ifp, hlen, fsize - hlen);
    } else {
      parse_tiff(ifp, 0);
      if (!strncmp(make,"NIKON",5) && filters == 0)
    make[0] = 0;
    }
  } else if (!memcmp (head, "\0MRM", 4))
    parse_minolta(ifp);
    else if (!memcmp (head, "\xff\xd8\xff\xe1", 4) &&
         !memcmp (head+6, "Exif", 4)) {
    fseek (ifp, 4, SEEK_SET);
    fseek (ifp, 4 + get2(ifp), SEEK_SET);
    if (fgetc(ifp) != 0xff)
      parse_tiff(ifp, 12);
  } else if (!memcmp (head, "BM", 2)) {
    data_offset = 0x1000;
    order = 0x4949;
    fseek (ifp, 38, SEEK_SET);
    if (get4(ifp) == 2834 && get4(ifp) == 2834) {
      strcpy (model, "BMQ");
      flip = 3;
      goto nucore;
    }
  } else if (!memcmp (head, "BR", 2)) {
    strcpy (model, "RAW");
nucore:
    strcpy (make, "Nucore");
    order = 0x4949;
    fseek (ifp, 10, SEEK_SET);
    data_offset += get4(ifp);
    get4(ifp);
    raw_width  = get4(ifp);
    raw_height = get4(ifp);
    if (model[0] == 'B' && raw_width == 2597) {
      raw_width++;
      data_offset -= 0x1000;
    }
  } else if (!memcmp (head+25, "ARECOYK", 7)) {
    strcpy (make, "Contax");
    strcpy (model,"N Digital");
    fseek (ifp, 60, SEEK_SET);
    camera_red  = get4(ifp);
    camera_red /= get4(ifp);
    camera_blue = get4(ifp);
    camera_blue = get4(ifp) / camera_blue;
  } else if (!memcmp (head, "FUJIFILM", 8)) {
    long data_offset_long;
    fseek (ifp, 84, SEEK_SET);
    parse_tiff(ifp, get4(ifp)+12);
    fseek (ifp, 100, SEEK_SET);
    pm_readbiglong(ifp, &data_offset_long);
    data_offset = data_offset_long;
  } else if (!memcmp (head, "DSC-Image", 9))
    parse_rollei(ifp);
  else if (!memcmp (head, "FOVb", 4))
    parse_foveon(ifp);
  else
      for (i=0; i < sizeof table / sizeof *table; i++)
          if (fsize == table[i].fsize) {
              strcpy (make,  table[i].make );
              strcpy (model, table[i].model);
              if (table[i].withjpeg)
                  parse_external_jpeg(inputFileName);
          }
  parse_mos(ifp, 8);
  parse_mos(ifp, 3472);

  for (i=0; i < sizeof corp / sizeof *corp; i++)
    if (strstr (make, corp[i]))     /* Simplify company names */
    strcpy (make, corp[i]);
  if (!strncmp (make, "KODAK", 5))
    make[16] = model[16] = 0;
  c = make + strlen(make);      /* Remove trailing spaces */
  while (*--c == ' ') *c = 0;
  c = model + strlen(model);
  while (*--c == ' ') *c = 0;
  i = strlen(make);         /* Remove make from model */
  if (!strncmp (model, make, i++))
    memmove (model, model+i, 64-i);
  make[63] = model[63] = model2[63] = 0;

  if (make[0] == 0) {
    pm_message ("unrecognized file format.");
    return 1;
  }

/*  File format is OK.  Do we know this camera? */
/*  Start with some useful defaults:           */

  top_margin = left_margin = 0;
  if ((raw_height | raw_width) < 0)
       raw_height = raw_width  = 0;
  height = raw_height;
  width  = raw_width;
  if (fuji_width) {
    width = height + fuji_width;
    height = width - 1;
    ymag = 1;
  }
  load_raw = NULL;
  if (is_dng) {
    strcat (model, " DNG");
    if (!filters)
      colors = tiff_samples;
    if (tiff_data_compression == 1)
      load_raw = adobe_dng_load_raw_nc;
    if (tiff_data_compression == 7)
      load_raw = adobe_dng_load_raw_lj;
    goto dng_skip;
  }

/*  We'll try to decode anything from Canon or Nikon. */

  if (!filters) filters = 0x94949494;
  if ((is_canon = !strcmp(make,"Canon")))
    load_raw = memcmp (head+6,"HEAPCCDR",8) ?
        lossless_jpeg_load_raw : canon_compressed_load_raw;
  if (!strcmp(make,"NIKON"))
    load_raw = nikon_is_compressed() ?
    nikon_compressed_load_raw : nikon_load_raw;

/* Set parameters based on camera name (for non-DNG files). */

  if (is_foveon) {
    if (!have64BitArithmetic)
      pm_error("This program was built without 64 bit arithmetic "
               "capability and the Foveon format requires it.");
    if (height*2 < width) ymag = 2;
    if (width < height) xmag = 2;
    filters = 0;
    load_raw = foveon_load_raw;
    foveon_coeff(&use_coeff, coeff);
  } else if (!strcmp(model,"PowerShot 600")) {
    height = 613;
    width  = 854;
    colors = 4;
    filters = 0xe1e4e1e4;
    load_raw = canon_600_load_raw;
  } else if (!strcmp(model,"PowerShot A5") ||
         !strcmp(model,"PowerShot A5 Zoom")) {
    height = 773;
    width  = 960;
    raw_width = 992;
    colors = 4;
    filters = 0x1e4e1e4e;
    load_raw = canon_a5_load_raw;
  } else if (!strcmp(model,"PowerShot A50")) {
    height =  968;
    width  = 1290;
    raw_width = 1320;
    colors = 4;
    filters = 0x1b4e4b1e;
    load_raw = canon_a5_load_raw;
  } else if (!strcmp(model,"PowerShot Pro70")) {
    height = 1024;
    width  = 1552;
    colors = 4;
    filters = 0x1e4b4e1b;
    load_raw = canon_a5_load_raw;
    black = 34;
  } else if (!strcmp(model,"PowerShot Pro90 IS")) {
    width  = 1896;
    colors = 4;
    filters = 0xb4b4b4b4;
  } else if (is_canon && raw_width == 2144) {
    height = 1550;
    width  = 2088;
    top_margin  = 8;
    left_margin = 4;
    if (!strcmp(model,"PowerShot G1")) {
      colors = 4;
      filters = 0xb4b4b4b4;
    }
  } else if (is_canon && raw_width == 2224) {
    height = 1448;
    width  = 2176;
    top_margin  = 6;
    left_margin = 48;
  } else if (is_canon && raw_width == 2376) {
    height = 1720;
    width  = 2312;
    top_margin  = 6;
    left_margin = 12;
  } else if (is_canon && raw_width == 2672) {
    height = 1960;
    width  = 2616;
    top_margin  = 6;
    left_margin = 12;
  } else if (is_canon && raw_width == 3152) {
    height = 2056;
    width  = 3088;
    top_margin  = 12;
    left_margin = 64;
    maximum = 0xfa0;
  } else if (is_canon && raw_width == 3160) {
    height = 2328;
    width  = 3112;
    top_margin  = 12;
    left_margin = 44;
  } else if (is_canon && raw_width == 3344) {
    height = 2472;
    width  = 3288;
    top_margin  = 6;
    left_margin = 4;
  } else if (!strcmp(model,"EOS D2000C")) {
    filters = 0x61616161;
    black = curve[200];
  } else if (!strcmp(model,"EOS-1D")) {
    raw_height = height = 1662;
    raw_width  = width  = 2496;
    data_offset = 288912;
    filters = 0x61616161;
  } else if (!strcmp(model,"EOS-1DS")) {
    raw_height = height = 2718;
    raw_width  = width  = 4082;
    data_offset = 289168;
    filters = 0x61616161;
  } else if (is_canon && raw_width == 3516) {
    top_margin  = 14;
    left_margin = 42;
    goto canon_cr2;
  } else if (is_canon && raw_width == 3596) {
    top_margin  = 12;
    left_margin = 74;
    goto canon_cr2;
  } else if (is_canon && raw_width == 5108) {
    top_margin  = 13;
    left_margin = 98;
    maximum = 0xe80;
canon_cr2:
    height = raw_height - top_margin;
    width  = raw_width - left_margin;
  } else if (!strcmp(model,"D1")) {
    camera_red  *= 256/527.0;
    camera_blue *= 256/317.0;
  } else if (!strcmp(model,"D1X")) {
    width  = 4024;
    ymag = 2;
  } else if (!strcmp(model,"D100")) {
    if (tiff_data_compression == 34713 && load_raw == nikon_load_raw)
      raw_width = (width += 3) + 3;
  } else if (!strcmp(model,"D2H")) {
    width  = 2482;
    left_margin = 6;
  } else if (!strcmp(model,"D2X")) {
    width  = 4312;
    pre_mul[0] = 1.514;
    pre_mul[2] = 1.727;
  } else if (fsize == 2465792) {
    height = 1203;
    width  = 1616;
    filters = 0x4b4b4b4b;
    colors = 4;
    load_raw = nikon_e950_load_raw;
    nikon_e950_coeff();
    pre_mul[0] = 1.18193;
    pre_mul[2] = 1.16452;
    pre_mul[3] = 1.17250;
  } else if (!strcmp(model,"E990")) {
    if (!timestamp && !nikon_e990()) goto cp_e995;
    height = 1540;
    width  = 2064;
    colors = 4;
    filters = 0xb4b4b4b4;
    nikon_e950_coeff();
    pre_mul[0] = 1.196;
    pre_mul[1] = 1.246;
    pre_mul[2] = 1.018;
  } else if (!strcmp(model,"E995")) {
cp_e995:
    strcpy (model, "E995");
    height = 1540;
    width  = 2064;
    colors = 4;
    filters = 0xe1e1e1e1;
  } else if (!strcmp(model,"E2100")) {
    if (!timestamp && !nikon_e2100()) goto cp_e2500;
    width = 1616;
    height = 1206;
    load_raw = nikon_e2100_load_raw;
    pre_mul[0] = 1.945;
    pre_mul[2] = 1.040;
  } else if (!strcmp(model,"E2500")) {
cp_e2500:
    strcpy (model, "E2500");
    width = 1616;
    height = 1204;
    colors = 4;
    filters = 0x4b4b4b4b;
  } else if (!strcmp(model,"E3700")) {
    if (!timestamp && pentax_optio33()) goto optio_33wr;
    height = 1542;
    width  = 2064;
    load_raw = nikon_e2100_load_raw;
    pre_mul[0] = 1.818;
    pre_mul[2] = 1.618;
  } else if (!strcmp(model,"Optio 33WR")) {
optio_33wr:
    strcpy (make, "PENTAX");
    strcpy (model,"Optio 33WR");
    height = 1542;
    width  = 2064;
    load_raw = nikon_e2100_load_raw;
    flip = 1;
    filters = 0x16161616;
    pre_mul[0] = 1.331;
    pre_mul[2] = 1.820;
  } else if (!strcmp(model,"E4300")) {
    if (!timestamp && minolta_z2()) goto dimage_z2;
    height = 1710;
    width  = 2288;
    filters = 0x16161616;
    pre_mul[0] = 508;
    pre_mul[1] = 256;
    pre_mul[2] = 322;
  } else if (!strcmp(model,"DiMAGE Z2")) {
dimage_z2:
    strcpy (make, "MINOLTA");
    strcpy (model,"DiMAGE Z2");
    height = 1710;
    width  = 2288;
    filters = 0x16161616;
    load_raw = nikon_e2100_load_raw;
    pre_mul[0] = 508;
    pre_mul[1] = 256;
    pre_mul[2] = 450;
  } else if (!strcmp(model,"E4500")) {
    height = 1708;
    width  = 2288;
    colors = 4;
    filters = 0xb4b4b4b4;
  } else if (!strcmp(model,"R-D1")) {
    tiff_data_compression = 34713;
    load_raw = nikon_load_raw;
  } else if (!strcmp(model,"FinePixS2Pro")) {
    height = 3584;
    width  = 3583;
    fuji_width = 2144;
    filters = 0x61616161;
    load_raw = fuji_s2_load_raw;
    black = 128;
    strcpy (model+7, " S2Pro");
  } else if (!strcmp(model,"FinePix S3Pro")) {
    height = 3583;
    width  = 3584;
    fuji_width = 2144;
    if (fsize > 18000000 && use_secondary)
      data_offset += 4352*2*1444;
    filters = 0x49494949;
    load_raw = fuji_s3_load_raw;
    maximum = 0xffff;
  } else if (!strcmp(model,"FinePix S5000")) {
    height = 2499;
    width  = 2500;
    fuji_width = 1423;
    filters = 0x49494949;
    load_raw = fuji_s5000_load_raw;
    maximum = 0x3e00;
  } else if (!strcmp(model,"FinePix S5100") ||
         !strcmp(model,"FinePix S5500")) {
    height = 1735;
    width  = 2304;
    data_offset += width*10;
    filters = 0x49494949;
    load_raw = unpacked_load_raw;
    maximum = 0xffff;
  } else if (!strcmp(model,"FinePix E550") ||
         !strcmp(model,"FinePix F810") ||
         !strcmp(model,"FinePix S7000")) {
    height = 3587;
    width  = 3588;
    fuji_width = 2047;
    filters = 0x49494949;
    load_raw = fuji_s7000_load_raw;
    maximum = 0x3e00;
  } else if (!strcmp(model,"FinePix F700") ||
         !strcmp(model,"FinePix S20Pro")) {
    height = 2523;
    width  = 2524;
    fuji_width = 1440;
    filters = 0x49494949;
    load_raw = fuji_f700_load_raw;
    maximum = 0x3e00;
  } else if (!strcmp(model,"Digital Camera KD-400Z")) {
    height = 1712;
    width  = 2312;
    raw_width = 2336;
    data_offset = 4034;
    fseek (ifp, 2032, SEEK_SET);
    goto konica_400z;
  } else if (!strcmp(model,"Digital Camera KD-510Z")) {
    data_offset = 4032;
    pre_mul[0] = 1.297;
    pre_mul[2] = 1.438;
    fseek (ifp, 2032, SEEK_SET);
    goto konica_510z;
  } else if (!strcasecmp(make,"MINOLTA")) {
    load_raw = unpacked_load_raw;
    maximum = 0xf7d;
    if (!strncmp(model,"DiMAGE A",8)) {
      if (!strcmp(model,"DiMAGE A200")) {
    filters = 0x49494949;
    tmp = camera_red;
    camera_red  = 1 / camera_blue;
    camera_blue = 1 / tmp;
      }
      load_raw = packed_12_load_raw;
      maximum = model[8] == '1' ? 0xf8b : 0xfff;
    } else if (!strncmp(model,"ALPHA",5) ||
           !strncmp(model,"DYNAX",5) ||
           !strncmp(model,"MAXXUM",6)) {
      load_raw = packed_12_load_raw;
      maximum = 0xffb;
    } else if (!strncmp(model,"DiMAGE G",8)) {
      if (model[8] == '4') {
    data_offset = 5056;
    pre_mul[0] = 1.602;
    pre_mul[2] = 1.441;
    fseek (ifp, 2078, SEEK_SET);
    height = 1716;
    width  = 2304;
      } else if (model[8] == '5') {
    data_offset = 4016;
    fseek (ifp, 1936, SEEK_SET);
konica_510z:
    height = 1956;
    width  = 2607;
    raw_width = 2624;
      } else if (model[8] == '6') {
    data_offset = 4032;
    fseek (ifp, 2030, SEEK_SET);
    height = 2136;
    width  = 2848;
      }
      filters = 0x61616161;
konica_400z:
      load_raw = unpacked_load_raw;
      maximum = 0x3df;
      order = 0x4d4d;
      camera_red   = get2(ifp);
      camera_blue  = get2(ifp);
      camera_red  /= get2(ifp);
      camera_blue /= get2(ifp);
    }
    if (pre_mul[0] == 1 && pre_mul[2] == 1) {
      pre_mul[0] = 1.42;
      pre_mul[2] = 1.25;
    }
  } else if (!strcmp(model,"*ist D")) {
    load_raw = unpacked_load_raw;
  } else if (!strcmp(model,"*ist DS")) {
    height--;
    load_raw = packed_12_load_raw;
  } else if (!strcmp(model,"Optio S")) {
    if (fsize == 3178560) {
      height = 1540;
      width  = 2064;
      load_raw = eight_bit_load_raw;
      camera_red  *= 4;
      camera_blue *= 4;
      pre_mul[0] = 1.391;
      pre_mul[2] = 1.188;
    } else {
      height = 1544;
      width  = 2068;
      raw_width = 3136;
      load_raw = packed_12_load_raw;
      maximum = 0xf7c;
      pre_mul[0] = 1.137;
      pre_mul[2] = 1.453;
    }
  } else if (!strncmp(model,"Optio S4",8)) {
    height = 1737;
    width  = 2324;
    raw_width = 3520;
    load_raw = packed_12_load_raw;
    maximum = 0xf7a;
    pre_mul[0] = 1.980;
    pre_mul[2] = 1.570;
  } else if (!strcmp(model,"STV680 VGA")) {
    height = 484;
    width  = 644;
    load_raw = eight_bit_load_raw;
    flip = 2;
    filters = 0x16161616;
    black = 16;
    pre_mul[0] = 1.097;
    pre_mul[2] = 1.128;
  } else if (!strcmp(make,"Phase One")) {
    switch (raw_height) {
      case 2060:
    strcpy (model, "LightPhase");
    height = 2048;
    width  = 3080;
    top_margin  = 5;
    left_margin = 22;
    pre_mul[0] = 1.331;
    pre_mul[2] = 1.154;
    break;
      case 2682:
    strcpy (model, "H10");
    height = 2672;
    width  = 4012;
    top_margin  = 5;
    left_margin = 26;
    break;
      case 4128:
    strcpy (model, "H20");
    height = 4098;
    width  = 4098;
    top_margin  = 20;
    left_margin = 26;
    pre_mul[0] = 1.963;
    pre_mul[2] = 1.430;
    break;
      case 5488:
    strcpy (model, "H25");
    height = 5458;
    width  = 4098;
    top_margin  = 20;
    left_margin = 26;
    pre_mul[0] = 2.80;
    pre_mul[2] = 1.20;
    }
    filters = top_margin & 1 ? 0x94949494 : 0x49494949;
    load_raw = phase_one_load_raw;
    maximum = 0xffff;
  } else if (!strcmp(model,"Ixpress")) {
    height = 4084;
    width  = 4080;
    filters = 0x49494949;
    load_raw = ixpress_load_raw;
    maximum = 0xffff;
    pre_mul[0] = 1.963;
    pre_mul[2] = 1.430;
  } else if (!strcmp(make,"Sinar") && !memcmp(head,"8BPS",4)) {
    fseek (ifp, 14, SEEK_SET);
    height = get4(ifp);
    width  = get4(ifp);
    filters = 0x61616161;
    data_offset = 68;
    load_raw = unpacked_load_raw;
    maximum = 0xffff;
  } else if (!strcmp(make,"Leaf")) {
    if (height > width)
      filters = 0x16161616;
    load_raw = unpacked_load_raw;
    maximum = 0x3fff;
    strcpy (model, "Valeo");
    if (raw_width == 2060) {
      filters = 0;
      load_raw = leaf_load_raw;
      maximum = 0xffff;
      strcpy (model, "Volare");
    }
  } else if (!strcmp(model,"DIGILUX 2") || !strcmp(model,"DMC-LC1")) {
    height = 1928;
    width  = 2568;
    data_offset = 1024;
    load_raw = unpacked_load_raw;
    maximum = 0xfff0;
  } else if (!strcmp(model,"E-1")) {
    filters = 0x61616161;
    load_raw = unpacked_load_raw;
    maximum = 0xfff0;
    black = 1024;
  } else if (!strcmp(model,"E-10")) {
    load_raw = unpacked_load_raw;
    maximum = 0xfff0;
    black = 2048;
  } else if (!strncmp(model,"E-20",4)) {
    load_raw = unpacked_load_raw;
    maximum = 0xffc0;
    black = 2560;
  } else if (!strcmp(model,"E-300")) {
    width -= 21;
    load_raw = olympus_e300_load_raw;
    if (fsize > 15728640) {
      load_raw = unpacked_load_raw;
      maximum = 0xfc30;
    } else
      black = 62;
  } else if (!strcmp(make,"OLYMPUS")) {
    load_raw = olympus_cseries_load_raw;
    if (!strcmp(model,"C5050Z") ||
    !strcmp(model,"C8080WZ"))
      filters = 0x16161616;
  } else if (!strcmp(model,"N Digital")) {
    height = 2047;
    width  = 3072;
    filters = 0x61616161;
    data_offset = 0x1a00;
    load_raw = packed_12_load_raw;
    maximum = 0xf1e;
  } else if (!strcmp(model,"DSC-F828")) {
    width = 3288;
    left_margin = 5;
    load_raw = sony_load_raw;
    filters = 0x9c9c9c9c;
    colors = 4;
    black = 491;
  } else if (!strcmp(model,"DSC-V3")) {
    width = 3109;
    left_margin = 59;
    load_raw = sony_load_raw;
  } else if (!strcasecmp(make,"KODAK")) {
    filters = 0x61616161;
    if (!strcmp(model,"NC2000F")) {
      width -= 4;
      left_margin = 1;
      for (i=176; i < 0x1000; i++)
    curve[i] = curve[i-1];
      pre_mul[0] = 1.509;
      pre_mul[2] = 2.686;
    } else if (!strcmp(model,"EOSDCS3B")) {
      width -= 4;
      left_margin = 2;
    } else if (!strcmp(model,"EOSDCS1")) {
      width -= 4;
      left_margin = 2;
    } else if (!strcmp(model,"DCS315C")) {
      black = 8;
    } else if (!strcmp(model,"DCS330C")) {
      black = 8;
    } else if (!strcmp(model,"DCS420")) {
      width -= 4;
      left_margin = 2;
    } else if (!strcmp(model,"DCS460")) {
      width -= 4;
      left_margin = 2;
    } else if (!strcmp(model,"DCS460A")) {
      width -= 4;
      left_margin = 2;
      colors = 1;
      filters = 0;
    } else if (!strcmp(model,"DCS520C")) {
      black = 180;
    } else if (!strcmp(model,"DCS560C")) {
      black = 188;
    } else if (!strcmp(model,"DCS620C")) {
      black = 180;
    } else if (!strcmp(model,"DCS620X")) {
      black = 185;
    } else if (!strcmp(model,"DCS660C")) {
      black = 214;
    } else if (!strcmp(model,"DCS660M")) {
      black = 214;
      colors = 1;
      filters = 0;
    } else if (!strcmp(model,"DCS760M")) {
      colors = 1;
      filters = 0;
    }
    switch (tiff_data_compression) {
    case 0:               /* No compression */
    case 1:
        load_raw = kodak_easy_load_raw;
        break;
    case 7:               /* Lossless JPEG */
        load_raw = lossless_jpeg_load_raw;
    case 32867:
        break;
    case 65000:           /* Kodak DCR compression */
        if (!have64BitArithmetic)
            pm_error("This program was built without 64 bit arithmetic "
                     "capability, and Kodak DCR compression requires it.");
        if (kodak_data_compression == 32803)
            load_raw = kodak_compressed_load_raw;
        else {
            load_raw = kodak_yuv_load_raw;
            height = (height+1) & -2;
            width  = (width +1) & -2;
            filters = 0;
        }
        break;
    default:
        pm_message ("%s %s uses unrecognized compression method %d.",
                    make, model, tiff_data_compression);
        return 1;
    }
    if (!strcmp(model,"DC20")) {
      height = 242;
      if (fsize < 100000) {
    width = 249;
    raw_width = 256;
      } else {
    width = 501;
    raw_width = 512;
      }
      data_offset = raw_width + 1;
      colors = 4;
      filters = 0x8d8d8d8d;
      kodak_dc20_coeff (1.0);
      pre_mul[1] = 1.179;
      pre_mul[2] = 1.209;
      pre_mul[3] = 1.036;
      load_raw = kodak_easy_load_raw;
    } else if (strstr(model,"DC25")) {
      strcpy (model, "DC25");
      height = 242;
      if (fsize < 100000) {
    width = 249;
    raw_width = 256;
    data_offset = 15681;
      } else {
    width = 501;
    raw_width = 512;
    data_offset = 15937;
      }
      colors = 4;
      filters = 0xb4b4b4b4;
      load_raw = kodak_easy_load_raw;
    } else if (!strcmp(model,"Digital Camera 40")) {
      strcpy (model, "DC40");
      height = 512;
      width = 768;
      data_offset = 1152;
      load_raw = kodak_radc_load_raw;
    } else if (strstr(model,"DC50")) {
      strcpy (model, "DC50");
      height = 512;
      width = 768;
      data_offset = 19712;
      load_raw = kodak_radc_load_raw;
    } else if (strstr(model,"DC120")) {
      strcpy (model, "DC120");
      height = 976;
      width = 848;
      if (tiff_data_compression == 7)
    load_raw = kodak_jpeg_load_raw;
      else
    load_raw = kodak_dc120_load_raw;
    }
  } else if (!strcmp(make,"Rollei")) {
    switch (raw_width) {
      case 1316:
    height = 1030;
    width  = 1300;
    top_margin  = 1;
    left_margin = 6;
    break;
      case 2568:
    height = 1960;
    width  = 2560;
    top_margin  = 2;
    left_margin = 8;
    }
    filters = 0x16161616;
    load_raw = rollei_load_raw;
    pre_mul[0] = 1.8;
    pre_mul[2] = 1.3;
  } else if (!strcmp(model,"PC-CAM 600")) {
    height = 768;
    data_offset = width = 1024;
    filters = 0x49494949;
    load_raw = eight_bit_load_raw;
    pre_mul[0] = 1.14;
    pre_mul[2] = 2.73;
  } else if (!strcmp(model,"QV-2000UX")) {
    height = 1208;
    width  = 1632;
    data_offset = width * 2;
    load_raw = eight_bit_load_raw;
  } else if (!strcmp(model,"QV-3*00EX")) {
    height = 1546;
    width  = 2070;
    raw_width = 2080;
    load_raw = eight_bit_load_raw;
  } else if (!strcmp(model,"QV-4000")) {
    height = 1700;
    width  = 2260;
    load_raw = unpacked_load_raw;
    maximum = 0xffff;
  } else if (!strcmp(model,"QV-5700")) {
    height = 1924;
    width  = 2576;
    load_raw = casio_qv5700_load_raw;
  } else if (!strcmp(model,"QV-R51")) {
    height = 1926;
    width  = 2576;
    raw_width = 3904;
    load_raw = packed_12_load_raw;
    pre_mul[0] = 1.340;
    pre_mul[2] = 1.672;
  } else if (!strcmp(model,"EX-Z55")) {
    height = 1960;
    width  = 2570;
    raw_width = 3904;
    load_raw = packed_12_load_raw;
    pre_mul[0] = 1.520;
    pre_mul[2] = 1.316;
  } else if (!strcmp(model,"EX-P600")) {
    height = 2142;
    width  = 2844;
    raw_width = 4288;
    load_raw = packed_12_load_raw;
    pre_mul[0] = 1.797;
    pre_mul[2] = 1.219;
  } else if (!strcmp(model,"EX-P700")) {
    height = 2318;
    width  = 3082;
    raw_width = 4672;
    load_raw = packed_12_load_raw;
    pre_mul[0] = 1.758;
    pre_mul[2] = 1.504;
  } else if (!strcmp(make,"Nucore")) {
    filters = 0x61616161;
    load_raw = unpacked_load_raw;
    if (width == 2598) {
      filters = 0x16161616;
      load_raw = nucore_load_raw;
      flip = 2;
    }
  }
  if (!use_coeff) adobe_coeff();
dng_skip:
  if (!load_raw || !height) {
    pm_message ("This program cannot handle data from %s %s.",
                make, model);
    return 1;
  }
#ifdef NO_JPEG
  if (load_raw == kodak_jpeg_load_raw) {
    pm_message ("decoder was not linked with libjpeg.");
    return 1;
  }
#endif
  if (!raw_height) raw_height = height;
  if (!raw_width ) raw_width  = width;
  if (use_camera_rgb && colors == 3)
      use_coeff = 0;
  if (use_coeff)         /* Apply user-selected color balance */
    for (i=0; i < colors; i++) {
      coeff[0][i] *= red_scale;
      coeff[2][i] *= blue_scale;
    }
  if (four_color_rgb && filters && colors == 3) {
    for (i=0; i < 32; i+=4) {
      if ((filters >> i & 15) == 9)
    filters |= 2 << i;
      if ((filters >> i & 15) == 6)
    filters |= 8 << i;
    }
    colors++;
    pre_mul[3] = pre_mul[1];
    if (use_coeff)
      for (i=0; i < 3; i++)
    coeff[i][3] = coeff[i][1] /= 2;
  }
  fseek (ifp, data_offset, SEEK_SET);

  *loadRawFnP = load_raw;

  return 0;
}
