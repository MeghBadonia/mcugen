/*
 * hct_solver.c
 * Material Color Utilities — C Implementation
 *
 * This file was automatically ported from the official Kotlin implementation
 * by Claude (Anthropic) as a faithful algorithmic translation into C.
 *
 * The algorithms and numeric constants in this file implement published
 * color-science specifications (CAM16, HCT, WCAG contrast, CIE L*a*b*).
 * Those mathematical formulas are not copyrightable facts.
 *
 * This C translation is released under the MIT License:
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "hct_solver.h"
#include "../utils/color_utils.h"
#include "../utils/math_utils.h"
#include <math.h>
#include <stdlib.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- Private constants ---- */

static const double kScaledDiscountFromLinrgb[3][3] = {
    {0.001200833568784504,  0.002389694492170889, 0.0002795742885861124},
    {0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398},
    {0.00010146692491640572,0.0005364214359186694, 0.0032979401770712076},
};

static const double kLinrgbFromScaledDiscount[3][3] = {
    { 1373.2198709594231,  -1100.4251190754821,   -7.278681089101213},
    {-271.815969077903,      559.6580465940733,  -32.46047482791194 },
    {   1.9622899599665666, -57.173814538844006,  308.7233197812385 },
};

static const double kYFromLinrgb[3] = {0.2126, 0.7152, 0.0722};

static const double kCriticalPlanes[255] = {
    0.015176349177441876, 0.045529047532325624, 0.07588174588720938,
    0.10623444424209313,  0.13658714259697685,  0.16693984095186062,
    0.19729253930674434,  0.2276452376616281,   0.2579979360165119,
    0.28835063437139563,  0.3188300904430532,   0.350925934958123,
    0.3848314933096426,   0.42057480301049466,  0.458183274052838,
    0.4976837250274023,   0.5391024159806381,   0.5824650784040898,
    0.6277969426914107,   0.6751227633498623,   0.7244668422128921,
    0.775853049866786,    0.829304845476233,    0.8848452951698498,
    0.942497089126609,    1.0022825574869039,   1.0642236851973577,
    1.1283421258858297,   1.1946592148522128,   1.2631959812511864,
    1.3339731595349034,   1.407011200216447,    1.4823302800086415,
    1.5599503113873272,   1.6398909516233677,   1.7221716113234105,
    1.8068114625156377,   1.8938294463134073,   1.9832442801866852,
    2.075074464868551,    2.1693382909216234,   2.2660538449872063,
    2.36523901573795,     2.4669114995532007,   2.5710888059345764,
    2.6777882626779785,   2.7870270208169257,   2.898822059350997,
    3.0131901897720907,   3.1301480604002863,   3.2497121605402226,
    3.3718988244681087,   3.4967242352587946,   3.624204428461639,
    3.754355295633311,    3.887192587735158,    4.022731918402185,
    4.160988767090289,    4.301978482107941,    4.445716283538092,
    4.592217266055746,    4.741496401646282,    4.893568542229298,
    5.048448422192488,    5.20615066083972,     5.3666897647573375,
    5.5300801301023865,   5.696336044816294,    5.865471690767354,
    6.037501145825082,    6.212438385869475,    6.390297286737924,
    6.571091626112461,    6.7548350853498045,   6.941541251256611,
    7.131223617812143,    7.323895587840543,    7.5195704746346665,
    7.7182615035334345,   7.919981813454504,    8.124744458384042,
    8.332562408825165,    8.543448553206703,    8.757415699253682,
    8.974476575321063,    9.194643831691977,    9.417930041841839,
    9.644347703669503,    9.873909240696694,   10.106627003236781,
   10.342513269534024,   10.58158024687427,    10.8238400726681,
   11.069304815507364,   11.317986476196008,   11.569896988756009,
   11.825048221409341,   12.083451977536606,   12.345119996613247,
   12.610063955123938,   12.878295467455942,   13.149826086772048,
   13.42466730586372,    13.702830557985108,   13.984327217668513,
   14.269168601521828,   14.55736596900856,    14.848930523210871,
   15.143873411576273,   15.44220572664832,    15.743938506781891,
   16.04908273684337,    16.35764934889634,    16.66964922287304,
   16.985093187232053,   17.30399201960269,    17.62635644741625,
   17.95219714852476,    18.281524751807332,   18.614349837764564,
   18.95068293910138,    19.290534541298456,   19.633915083172692,
   19.98083495742689,    20.331304511189067,   20.685334046541502,
   21.042933821039977,   21.404114048223256,   21.76888489811322,
   22.137256497705877,   22.50923893145328,    22.884842241736916,
   23.264076429332462,   23.6469514538663,     24.033477234264016,
   24.42366364919083,    24.817520537484558,   25.21505769858089,
   25.61628489293138,    26.021211842414342,   26.429848230738664,
   26.842203703840827,   27.258287870275353,   27.678110301598522,
   28.10168053274597,    28.529008062403893,   28.96010235337422,
   29.39497283293396,    29.83362889318845,    30.276079891419332,
   30.722335150426627,   31.172403958865512,   31.62629557157785,
   32.08401920991837,    32.54558406207592,    33.010999283389665,
   33.4802739966603,     33.953417292456834,   34.430438229418264,
   34.911345834551085,   35.39614910352207,    35.88485700094671,
   36.37747846067349,    36.87402238606382,    37.37449765026789,
   37.87891309649659,    38.38727753828926,    38.89959975977785,
   39.41588851594697,    39.93615253289054,    40.460400508064545,
   40.98864111053629,    41.520882981230194,   42.05713473317016,
   42.597404951718396,   43.141702194811224,   43.6900349931913,
   44.24241185063697,    44.798841244188324,   45.35933162437017,
   45.92389141541209,    46.49252901546552,    47.065252796817916,
   47.64207110610409,    48.22299226451468,    48.808024568002054,
   49.3971762874833,     49.9904556690408,     50.587870934119984,
   51.189430279724725,   51.79514187861014,    52.40501387947288,
   53.0190544071392,     53.637271562750364,   54.259673423945976,
   54.88626804504493,    55.517063457223934,   56.15206766869424,
   56.79128866487574,    57.43473440856916,    58.08241284012621,
   58.734331877617365,   59.39049941699807,    60.05092333227251,
   60.715611475655585,   61.38457167773311,    62.057811747619894,
   62.7353394731159,     63.417162620860914,   64.10328893648692,
   64.79372614476921,    65.48848194977529,    66.18756403501224,
   66.89098006357258,    67.59873767827808,    68.31084450182222,
   69.02730813691093,    69.74813616640164,    70.47333615344107,
   71.20291564160104,    71.93688215501312,    72.67524319850172,
   73.41800625771542,    74.16517879925733,    74.9167682708136,
   75.67278210128072,    76.43322770089146,    77.1981124613393,
   77.96744375590167,    78.74122893956174,    79.51947534912904,
   80.30219030335869,    81.08938110306934,    81.88105503125999,
   82.67721935322541,    83.4778813166706,     84.28304815182372,
   85.09272707154808,    85.90692527145302,    86.72564993000343,
   87.54890820862819,    88.3767072518277,     89.2090541872801,
   90.04595612594655,    90.88742016217518,    91.73345337380438,
   92.58406282226491,    93.43925555268066,    94.29903859396902,
   95.16341895893969,    96.03240364439274,    96.9059996312159,
   97.78421388448044,    98.6670533535366,     99.55452497210776,
};

/* ---- Private helpers ---- */

static double sanitize_radians(double angle) {
  return fmod(angle + M_PI * 8.0, M_PI * 2.0);
}

static double true_delinearized(double rgb_component) {
  double normalized = rgb_component / 100.0;
  double delinearized;
  if (normalized <= 0.0031308) {
    delinearized = normalized * 12.92;
  } else {
    delinearized = 1.055 * pow(normalized, 1.0 / 2.4) - 0.055;
  }
  return delinearized * 255.0;
}

static double chromatic_adaptation(double component) {
  double af = pow(fabs(component), 0.42);
  double sgn = (component > 0.0) ? 1.0 : ((component < 0.0) ? -1.0 : 0.0);
  return sgn * 400.0 * af / (af + 27.13);
}

static double hue_of(const double linrgb[3]) {
  double scaled[3];
  MathUtils_matrixMultiply(linrgb, kScaledDiscountFromLinrgb, scaled);
  double rA = chromatic_adaptation(scaled[0]);
  double gA = chromatic_adaptation(scaled[1]);
  double bA = chromatic_adaptation(scaled[2]);
  double a  = (11.0 * rA - 12.0 * gA + bA) / 11.0;
  double b  = (rA + gA - 2.0 * bA) / 9.0;
  return atan2(b, a);
}

static int are_in_cyclic_order(double a, double b, double c) {
  double delta_ab = sanitize_radians(b - a);
  double delta_ac = sanitize_radians(c - a);
  return delta_ab < delta_ac;
}

static double intercept(double source, double mid, double target) {
  return (mid - source) / (target - source);
}

static void lerp_point(const double source[3], double t,
                        const double target[3], double out[3]) {
  out[0] = source[0] + (target[0] - source[0]) * t;
  out[1] = source[1] + (target[1] - source[1]) * t;
  out[2] = source[2] + (target[2] - source[2]) * t;
}

static void set_coordinate(const double source[3], double coordinate,
                             const double target[3], int axis,
                             double out[3]) {
  double t = intercept(source[axis], coordinate, target[axis]);
  lerp_point(source, t, target, out);
}

static int is_bounded(double x) { return x >= 0.0 && x <= 100.0; }

/* Returns 1 on success and fills out[3], returns 0 if out of gamut. */
static int nth_vertex(double y, int n, double out[3]) {
  double kR = kYFromLinrgb[0];
  double kG = kYFromLinrgb[1];
  double kB = kYFromLinrgb[2];
  double coord_a = (n % 4 <= 1) ? 0.0 : 100.0;
  double coord_b = (n % 2 == 0) ? 0.0 : 100.0;
  if (n < 4) {
    double g = coord_a, b = coord_b;
    double r = (y - g * kG - b * kB) / kR;
    if (!is_bounded(r)) return 0;
    out[0] = r; out[1] = g; out[2] = b;
  } else if (n < 8) {
    double b = coord_a, r = coord_b;
    double g = (y - r * kR - b * kB) / kG;
    if (!is_bounded(g)) return 0;
    out[0] = r; out[1] = g; out[2] = b;
  } else {
    double r = coord_a, g = coord_b;
    double b = (y - r * kR - g * kG) / kB;
    if (!is_bounded(b)) return 0;
    out[0] = r; out[1] = g; out[2] = b;
  }
  return 1;
}

static void bisect_to_segment(double y, double target_hue,
                               double left[3], double right[3]) {
  double left_hue = 0.0, right_hue = 0.0;
  int initialized = 0, uncut = 1;
  for (int n = 0; n <= 11; n++) {
    double mid[3];
    if (!nth_vertex(y, n, mid)) continue;
    double mid_hue = hue_of(mid);
    if (!initialized) {
      left[0] = mid[0]; left[1] = mid[1]; left[2] = mid[2];
      right[0] = mid[0]; right[1] = mid[1]; right[2] = mid[2];
      left_hue = mid_hue;
      right_hue = mid_hue;
      initialized = 1;
    } else if (uncut || are_in_cyclic_order(left_hue, mid_hue, right_hue)) {
      uncut = 0;
      if (are_in_cyclic_order(left_hue, target_hue, mid_hue)) {
        right[0] = mid[0]; right[1] = mid[1]; right[2] = mid[2];
        right_hue = mid_hue;
      } else {
        left[0] = mid[0]; left[1] = mid[1]; left[2] = mid[2];
        left_hue = mid_hue;
      }
    }
  }
}

static void midpoint(const double a[3], const double b[3], double out[3]) {
  out[0] = (a[0] + b[0]) / 2.0;
  out[1] = (a[1] + b[1]) / 2.0;
  out[2] = (a[2] + b[2]) / 2.0;
}

static int critical_plane_below(double x) {
  return (int)floor(x - 0.5);
}

static int critical_plane_above(double x) {
  return (int)ceil(x - 0.5);
}

static void bisect_to_limit(double y, double target_hue, double out[3]) {
  double left[3], right[3];
  bisect_to_segment(y, target_hue, left, right);
  double left_hue = hue_of(left);
  for (int axis = 0; axis <= 2; axis++) {
    if (left[axis] == right[axis]) continue;
    int l_plane, r_plane;
    if (left[axis] < right[axis]) {
      l_plane = critical_plane_below(true_delinearized(left[axis]));
      r_plane = critical_plane_above(true_delinearized(right[axis]));
    } else {
      l_plane = critical_plane_above(true_delinearized(left[axis]));
      r_plane = critical_plane_below(true_delinearized(right[axis]));
    }
    for (int i = 0; i < 8; i++) {
      if (abs(r_plane - l_plane) <= 1) break;
      int m_plane = (int)floor((l_plane + r_plane) / 2.0);
      double mid_coord = kCriticalPlanes[m_plane];
      double mid[3];
      set_coordinate(left, mid_coord, right, axis, mid);
      double mid_hue = hue_of(mid);
      if (are_in_cyclic_order(left_hue, target_hue, mid_hue)) {
        right[0] = mid[0]; right[1] = mid[1]; right[2] = mid[2];
        r_plane = m_plane;
      } else {
        left[0] = mid[0]; left[1] = mid[1]; left[2] = mid[2];
        left_hue = mid_hue;
        l_plane = m_plane;
      }
    }
  }
  midpoint(left, right, out);
}

static double inverse_chromatic_adaptation(double adapted) {
  double adapted_abs = fabs(adapted);
  double base = fmax(0.0, 27.13 * adapted_abs / (400.0 - adapted_abs));
  double sgn = (adapted > 0.0) ? 1.0 : ((adapted < 0.0) ? -1.0 : 0.0);
  return sgn * pow(base, 1.0 / 0.42);
}

static int find_result_by_j(double hue_radians, double chroma, double y) {
  double j = sqrt(y) * 11.0;
  const ViewingConditions *vc = ViewingConditions_DEFAULT();
  double t_inner_coeff =
      1.0 / pow(1.64 - pow(0.29, vc->n), 0.73);
  double eHue = 0.25 * (cos(hue_radians + 2.0) + 3.8);
  double p1   = eHue * (50000.0 / 13.0) * vc->nc * vc->ncb;
  double hSin = sin(hue_radians);
  double hCos = cos(hue_radians);

  for (int round = 0; round <= 4; round++) {
    double j_norm = j / 100.0;
    double alpha  = (chroma == 0.0 || j == 0.0) ? 0.0 : chroma / sqrt(j_norm);
    double t      = pow(alpha * t_inner_coeff, 1.0 / 0.9);
    double ac     = vc->aw * pow(j_norm, 1.0 / vc->c / vc->z);
    double p2     = ac / vc->nbb;
    double gamma  = 23.0 * (p2 + 0.305) * t /
                    (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);
    double a  = gamma * hCos;
    double b  = gamma * hSin;
    double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
    double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
    double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;
    double rCS = inverse_chromatic_adaptation(rA);
    double gCS = inverse_chromatic_adaptation(gA);
    double bCS = inverse_chromatic_adaptation(bA);
    double row[3] = {rCS, gCS, bCS};
    double linrgb[3];
    MathUtils_matrixMultiply(row, kLinrgbFromScaledDiscount, linrgb);
    if (linrgb[0] < 0.0 || linrgb[1] < 0.0 || linrgb[2] < 0.0) return 0;
    double kR = kYFromLinrgb[0];
    double kG = kYFromLinrgb[1];
    double kB = kYFromLinrgb[2];
    double fnj = kR * linrgb[0] + kG * linrgb[1] + kB * linrgb[2];
    if (fnj <= 0.0) return 0;
    if (round == 4 || fabs(fnj - y) < 0.002) {
      if (linrgb[0] > 100.01 || linrgb[1] > 100.01 || linrgb[2] > 100.01)
        return 0;
      return ColorUtils_argbFromLinrgb(linrgb);
    }
    j -= (fnj - y) * j / (2.0 * fnj);
  }
  return 0;
}

/* ---- Public API ---- */

int HctSolver_solveToInt(double hue_degrees, double chroma, double lstar) {
  if (chroma < 0.0001 || lstar < 0.0001 || lstar > 99.9999) {
    return ColorUtils_argbFromLstar(lstar);
  }
  double hue_radians =
      MathUtils_sanitizeDegreesDouble(hue_degrees) / 180.0 * M_PI;
  double y = ColorUtils_yFromLstar(lstar);
  int exact = find_result_by_j(hue_radians, chroma, y);
  if (exact != 0) return exact;
  double linrgb[3];
  bisect_to_limit(y, hue_radians, linrgb);
  return ColorUtils_argbFromLinrgb(linrgb);
}

Cam16 HctSolver_solveToCam(double hue_degrees, double chroma, double lstar) {
  return Cam16_fromInt(HctSolver_solveToInt(hue_degrees, chroma, lstar));
}
