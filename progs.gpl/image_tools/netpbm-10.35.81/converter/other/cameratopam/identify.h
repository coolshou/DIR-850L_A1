typedef void (*loadRawFn)();

int
identify(FILE *       const ifp,
         bool         const use_secondary,
         bool         const use_camera_rgb,
         float        const red_scale,
         float        const blue_scale,
         unsigned int const four_color_rgb,
         const char * const inputFileName,
         loadRawFn *  const loadRawFnP);
