

static void
build_aa_samples(GLint xSamples, GLint ySamples,
                 GLfloat samples[][2])
{
   const GLfloat dx = 1.0F / (GLfloat) xSamples;
   const GLfloat dy = 1.0F / (GLfloat) ySamples;
   GLint x, y;
   GLint i;

   i = 4;
   for (x = 0; x < xSamples; x++) {
      for (y = 0; y < ySamples; y++) {
         GLint j;
         if (x == 0 && y == 0) {
            /* lower left */
            j = 0;
         }
         else if (x == xSamples - 1 && y == 0) {
            /* lower right */
            j = 1;
         }
         else if (x == 0 && y == ySamples - 1) {
            /* upper left */
            j = 2;
         }
         else if (x == xSamples - 1 && y == ySamples - 1) {
            /* upper right */
            j = 3;
         }
         else {
            j = i++;
         }
         samples[j][0] = x * dx;
         samples[j][1] = y * dy;
      }
   }
}
