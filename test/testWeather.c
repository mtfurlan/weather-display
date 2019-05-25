#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "parseWeather.h"
#include "yy_color_converter.h"

int main(void)
{
    FILE *f = fopen("darksky.json", "r");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char* json = malloc(fsize + 1);
    fread(json, 1, fsize, f);
    fclose(f);

    json[fsize] = 0;
    cJSON* weather_cjson = cJSON_Parse(json);
    printf("Read JSON\n");

    weather_t weather;

    parseWeather(weather_cjson, &weather);

    for(int i = 0; i < 25; ++i) {
        char sun = ' ';
        if(weather.hourly[i].timestamp < weather.sunrise_ts && weather.sunrise_ts < weather.hourly[i].timestamp + 60*60) {
            sun = '^';
        } else if(weather.hourly[i].timestamp < weather.sunset_ts && weather.sunset_ts < weather.hourly[i].timestamp + 60*60) {
            sun = 'v';
        }
        printf("[%2.1f %c]", weather.hourly[i].temp, sun);
        //localtime_r(&(weather.hourly[i].timestamp), &(weather.hourly[i].time));
        //printf("%04d-%02d-%02d %02d:%02d:%02d -400 %f %c\n", weather.hourly[i].time.tm_year+1900, weather.hourly[i].time.tm_mon+1, weather.hourly[i].time.tm_mday, weather.hourly[i].time.tm_hour, weather.hourly[i].time.tm_min, weather.hourly[i].time.tm_sec, weather.hourly[i].temp, sun);
    }
    printf("\n");



    //HCL/LCH
    //Hue: angle
    //Chroma: distance
    //Luminance: Y axis

    //CIELCHab2CIELab(41.6567159358981, 87.0898452442239, 40.0001579064637, &L, &a, &b);
    //CIELab2CIEXYZ(yy_illuminant_D50,
    //               L, a, b,
    //               &X, &Y, &Z);
    //CIEXYZ2RGB(yy_illuminant_D50, yy_gamma_1_0,
    //            yy_rgbspace_BestRGB, yy_adaption_NONE,
    //            X, Y, Z,
    //            &R, &G, &B);

    testColor(255, 0, 0);
    testColor(0, 255, 0);
    testColor(0, 0, 255);
    testColor(255, 255, 255);

    CGFloat l, c, h;
    CGFloat R, G, B;
    l = 30;
    c = 100;
    h = 0;
    printf("LCH: %f, %f, %f\n", l, c, h);
    LCH2RGB(l, c, h, &R, &G, &B);
    printf("RGB: %f, %f, %f\n", R*255, G*255, B*255);
    RGB2LCH(R, G, B, &l, &c, &h);
    printf("LCH: %f, %f, %f\n\n", l, c, h);
    return 0;
}
int lineIntersection(
double Ax, double Ay,
double Bx, double By,
double Cx, double Cy,
double Dx, double Dy,
double *X, double *Y) {

  double  distAB, theCos, theSin, newX, ABpos ;

  //  Fail if either line is undefined.
  if (Ax==Bx && Ay==By || Cx==Dx && Cy==Dy) return 1;

  //  (1) Translate the system so that point A is on the origin.
  Bx-=Ax; By-=Ay;
  Cx-=Ax; Cy-=Ay;
  Dx-=Ax; Dy-=Ay;

  //  Discover the length of segment A-B.
  distAB=sqrt(Bx*Bx+By*By);

  //  (2) Rotate the system so that point B is on the positive X axis.
  theCos=Bx/distAB;
  theSin=By/distAB;
  newX=Cx*theCos+Cy*theSin;
  Cy  =Cy*theCos-Cx*theSin; Cx=newX;
  newX=Dx*theCos+Dy*theSin;
  Dy  =Dy*theCos-Dx*theSin; Dx=newX;

  //  Fail if the lines are parallel.
  if (Cy==Dy) return 1;

  //  (3) Discover the position of the intersection point along line A-B.
  ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

  //  (4) Apply the discovered position to line A-B in the original coordinate system.
  *X=Ax+ABpos*theCos;
  *Y=Ay+ABpos*theSin;

  //  Success.
  return 0;
}
double lineIntercept(double h, double a1, double r1, double a2, double r2)
{
    double x1 = r1* cos(a1);
    double y1 = r1* sin(a1);
    double x2 = r2* cos(a2);
    double y2 = r2* sin(a2);

    double x3 = 500* cos(h);
    double y3 = 500* sin(h);

    double X, Y;

    lineIntersection(x1, x2, y1, y2, 0, 0, x3, y3, &X, &Y);

    return sqrt(pow(X, 2) + pow(Y, 2));

}
double maxChromaForHue(double h)
{
    //triangle
    //red  : 36.31, 159.87
    double redA = 36.314239;
    double redD = 159.870807;
    //green: 138.56 204.76
    double greenA = 138.564686;
    double greenD = 204.756793;
    //blue : 304.441 162.86
    double blueA = 304.441534;
    double blueD = 162.864741;
    if(h < redA && h+360 > blueA) {
        return lineIntercept(h, redA, redD, blueA, blueD);
    }
    if(h > redA && h < greenA) {
        return lineIntercept(h, redA, redD, greenA, greenD);
    }
    if(h > greenA && h < blueA) {
        return lineIntercept(h, blueA, blueD, greenA, greenD);
    }
}

void testColor(uint8_t r, uint8_t g, uint8_t b)
{
    CGFloat l, c, h;
    CGFloat R, G, B;
    R = (double)r/255;
    G = (double)g/255;
    B = (double)b/255;
    printf("RGB: %f, %f, %f\n", R, G, B);
    RGB2LCH(R, G, B, &l, &c, &h);
    printf("LCH: %f, %f, %f\n", l, c, h);
    LCH2RGB(l, c, h, &R, &G, &B);
    printf("RGB: %f, %f, %f\n\n", R, G, B);
}
void LCH2RGB(CGFloat l, CGFloat c, CGFloat h, CGFloat* R, CGFloat* G, CGFloat* B) {
    CGFloat L, a, b;
    CGFloat X, Y, Z;
    CIELCHab2CIELab(l, c, h, &L, &a, &b);
    CIELab2CIEXYZ(yy_illuminant_D50,
                   L, a, b,
                   &X, &Y, &Z);
    CIEXYZ2RGB(yy_illuminant_D50, yy_gamma_1_0,
                yy_rgbspace_BestRGB, yy_adaption_NONE,
                X, Y, Z,
                R, G, B);

}
void RGB2LCH(CGFloat R, CGFloat G, CGFloat B, CGFloat* l, CGFloat* c, CGFloat* h) {
    CGFloat L, a, b;
    CGFloat X, Y, Z;

    RGB2CIEXYZ(yy_illuminant_D50, yy_gamma_1_0,
                yy_rgbspace_BestRGB, yy_adaption_NONE,
                R, G, B,
                &X, &Y, &Z);
    CIEXYZ2CIELab(yy_illuminant_D50,
                   X, Y, Z,
                   &L, &a, &b);
    CIELab2CIELCHab(L, a, b, l, c, h);
}
