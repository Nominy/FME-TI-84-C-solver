#include <ti/getcsc.h>
#include <ti/screen.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const double MU0 = 4.0 * M_PI * 1e-7;          // H/m
static const double EPS0 = 8.854e-12;                 // F/m
static const double E_CHARGE = 1.602e-19;             // C
static const double K_B = 1.38064852e-23;             // J/K
static const double LORENZ = 2.44e-8;                 // V^2/K^2

static const char *ctx_formula = NULL;
static const char *ctx_target = NULL;

static void put_line(const char *text, int maxlen) {
    char buf[32];
    int len = 0;
    while (text[len] && len < maxlen && len < (int)sizeof(buf) - 1) {
        buf[len] = text[len];
        ++len;
    }
    buf[len] = '\0';
    os_PutStrFull(buf);
}

static void set_context(const char *formula, const char *target) {
    ctx_formula = formula;
    ctx_target = target;
}

static void wait_key(void) {
    while (!os_GetCSC()) {
    }
}

static bool ask_double(const char *prompt, double *out) {
    char buf[24] = {0};
    char label[32];
    int i = 0;
    for (; prompt[i] && i < (int)sizeof(label) - 1; ++i) {
        char c = prompt[i];
        if (c == '[') c = '(';
        if (c == ']') c = ')';
        label[i] = c;
    }
    label[i] = '\0';
    os_ClrHome();
    if (ctx_formula) {
        put_line(ctx_formula, 31);
        os_NewLine();
    }
    if (ctx_target) {
        os_PutStrFull("Solve: ");
        put_line(ctx_target, 31);
        os_NewLine();
    }
    put_line(label, 31);
    os_NewLine();
    os_PutStrFull("Blank=skip Enter=OK");
    os_NewLine();
    os_GetStringInput("Value: ", buf, sizeof(buf));
    if (buf[0] == '\0') {
        return false;
    }
    char *end = NULL;
    double val = strtod(buf, &end);
    if (end == buf) {
        os_PutStrFull("Invalid number.");
        os_NewLine();
        os_PutStrFull("Press any key...");
        wait_key();
        return false;
    }
    *out = val;
    return true;
}

static int menu(const char *title, const char *const *options, int count) {
    const int visible = 6;
    int sel = 0;
    int top = 0;

    while (1) {
        os_ClrHome();
        put_line(title, 31);
        os_NewLine();

        for (int i = 0; i < visible && (top + i) < count; ++i) {
            int idx = top + i;
            os_PutStrFull(idx == sel ? "> " : "  ");
            put_line(options[idx], 31);
            os_NewLine();
        }
        os_PutStrFull("Enter=OK  Clear=Back");
        if (count > visible) {
            os_NewLine();
            os_PutStrFull("Up/Down=scroll");
        }

        uint8_t key;
        do {
            key = os_GetCSC();
        } while (!key);

        if (key == sk_Clear) {
            return -1;
        } else if (key == sk_Enter) {
            return sel;
        } else if (key == sk_Up) {
            sel = (sel == 0) ? (count - 1) : (sel - 1);
        } else if (key == sk_Down) {
            sel = (sel + 1 == count) ? 0 : (sel + 1);
        } else {
            continue;
        }

        if (sel < top) {
            top = sel;
        } else if (sel >= top + visible) {
            top = sel - visible + 1;
        }
    }
}

static void show_result(const char *label, double value, const char *unit) {
    char line[40];
    os_ClrHome();
    snprintf(line, sizeof(line), "%s = %.6g %s", label, value, unit);
    os_PutStrFull(line);
    os_NewLine();
    os_PutStrFull("Press any key...");
    wait_key();
}

static void show_error(void) {
    os_ClrHome();
    os_PutStrFull("Not enough data.");
    os_NewLine();
    os_PutStrFull("Press any key...");
    wait_key();
}

static void force_block(void) {
    static const char *units = "N";
    static const char *labels[] = {"F", "m", "a", "e", "E", "v", "tau"};
    int target = menu("Force F/ma/eE", labels, 7);
    if (target < 0) return;
    set_context("Force: F=m*a=e*E", labels[target]);
    double F, m, a, e, E, v, tau;
    switch (target) {
        case 0: // F
            if (ask_double("m[kg]? ", &m) && ask_double("a[m/s^2]? ", &a)) {
                show_result("F", m * a, units);
                return;
            }
            if (ask_double("e[C]? ", &e) && ask_double("E[V/m]? ", &E)) {
                show_result("F", e * E, units);
                return;
            }
            break;
        case 1: // m
            if (ask_double("F[N]? ", &F) && ask_double("a[m/s^2]? ", &a) && a != 0) {
                show_result("m", F / a, "kg");
                return;
            }
            break;
        case 2: // a
            if (ask_double("F[N]? ", &F) && ask_double("m[kg]? ", &m) && m != 0) {
                show_result("a", F / m, "m/s^2");
                return;
            }
            if (ask_double("v[m/s]? ", &v) && ask_double("tau[s]? ", &tau) && tau != 0) {
                show_result("a", v / tau, "m/s^2");
                return;
            }
            break;
        case 3: // e
            if (ask_double("F[N]? ", &F) && ask_double("E[V/m]? ", &E) && E != 0) {
                show_result("e", F / E, "C");
                return;
            }
            break;
        case 4: // E
            if (ask_double("F[N]? ", &F) && ask_double("e[C]? ", &e) && e != 0) {
                show_result("E", F / e, "V/m");
                return;
            }
            break;
        case 5: // v
            if (ask_double("a[m/s^2]? ", &a) && ask_double("tau[s]? ", &tau)) {
                show_result("v", a * tau, "m/s");
                return;
            }
            break;
        case 6: // tau
            if (ask_double("v[m/s]? ", &v) && ask_double("a[m/s^2]? ", &a) && a != 0) {
                show_result("tau", v / a, "s");
                return;
            }
            break;
    }
    show_error();
}

static void current_density_block(void) {
    static const char *labels[] = {"j", "n", "v", "gamma", "E", "rho"};
    int target = menu("j=en v=E/rho", labels, 6);
    if (target < 0) return;
    set_context("j = e*n*v = gamma*E", labels[target]);
    double j, n, v, gamma, E, rho;
    switch (target) {
        case 0: // j
            if (ask_double("n[1/m^3]? ", &n) && ask_double("v[m/s]? ", &v)) {
                show_result("j", E_CHARGE * n * v, "A/m^2");
                return;
            }
            if (ask_double("gamma[S/m]? ", &gamma) && ask_double("E[V/m]? ", &E)) {
                show_result("j", gamma * E, "A/m^2");
                return;
            }
            if (ask_double("E[V/m]? ", &E) && ask_double("rho[ohm*m]? ", &rho) && rho != 0) {
                show_result("j", E / rho, "A/m^2");
                return;
            }
            break;
        case 1: // n
            if (ask_double("j[A/m^2]? ", &j) && ask_double("v[m/s]? ", &v) && v != 0) {
                show_result("n", j / (E_CHARGE * v), "1/m^3");
                return;
            }
            break;
        case 2: // v
            if (ask_double("j[A/m^2]? ", &j) && ask_double("n[1/m^3]? ", &n) && n != 0) {
                show_result("v", j / (E_CHARGE * n), "m/s");
                return;
            }
            break;
        case 3: // gamma
            if (ask_double("j[A/m^2]? ", &j) && ask_double("E[V/m]? ", &E) && E != 0) {
                show_result("gamma", j / E, "S/m");
                return;
            }
            break;
        case 4: // E
            if (ask_double("j[A/m^2]? ", &j) && ask_double("gamma[S/m]? ", &gamma) && gamma != 0) {
                show_result("E", j / gamma, "V/m");
                return;
            }
            if (ask_double("j[A/m^2]? ", &j) && ask_double("rho[ohm*m]? ", &rho)) {
                show_result("E", j * rho, "V/m");
                return;
            }
            break;
        case 5: // rho
            if (ask_double("E[V/m]? ", &E) && ask_double("j[A/m^2]? ", &j) && j != 0) {
                show_result("rho", E / j, "ohm*m");
                return;
            }
            break;
    }
    show_error();
}

static void mobility_block(void) {
    static const char *labels[] = {"u", "v", "E"};
    int target = menu("Mobility u=v/E", labels, 3);
    if (target < 0) return;
    set_context("Mobility: u = v / E", labels[target]);
    double u, v, E;
    switch (target) {
        case 0:
            if (ask_double("v[m/s]? ", &v) && ask_double("E[V/m]? ", &E) && E != 0) {
                show_result("u", v / E, "m^2/Vs");
                return;
            }
            break;
        case 1:
            if (ask_double("u[m^2/Vs]? ", &u) && ask_double("E[V/m]? ", &E)) {
                show_result("v", u * E, "m/s");
                return;
            }
            break;
        case 2:
            if (ask_double("v[m/s]? ", &v) && ask_double("u[m^2/Vs]? ", &u) && u != 0) {
                show_result("E", v / u, "V/m");
                return;
            }
            break;
    }
    show_error();
}

static void tcr_block(void) {
    static const char *labels[] = {"R_T", "R_20", "alpha20", "T"};
    int target = menu("R_T=R_20(1+alpha)", labels, 4);
    if (target < 0) return;
    set_context("R_T = R_20*(1+alpha*(T-20))", labels[target]);
    double RT, R20, alpha, T;
    switch (target) {
        case 0:
            if (ask_double("R20[ohm]? ", &R20) && ask_double("alpha[1/K]? ", &alpha) && ask_double("T[C]? ", &T)) {
                show_result("R_T", R20 * (1 + alpha * (T - 20)), "ohm");
                return;
            }
            break;
        case 1:
            if (ask_double("R_T[ohm]? ", &RT) && ask_double("alpha[1/K]? ", &alpha) && ask_double("T[C]? ", &T)) {
                double denom = 1 + alpha * (T - 20);
                if (denom != 0) {
                    show_result("R_20", RT / denom, "ohm");
                    return;
                }
            }
            break;
        case 2:
            if (ask_double("R_T[ohm]? ", &RT) && ask_double("R_20[ohm]? ", &R20) && ask_double("T[C]? ", &T) && T != 20 && R20 != 0) {
                show_result("alpha20", (RT / R20 - 1) / (T - 20), "1/K");
                return;
            }
            break;
        case 3:
            if (ask_double("R_T[ohm]? ", &RT) && ask_double("R_20[ohm]? ", &R20) && ask_double("alpha[1/K]? ", &alpha) && R20 != 0 && alpha != 0) {
                show_result("T", (RT / R20 - 1) / alpha + 20, "C");
                return;
            }
            break;
    }
    show_error();
}

static void resistance_block(void) {
    static const char *labels[] = {"R", "rho", "l", "S"};
    int target = menu("R=rho*l/S", labels, 4);
    if (target < 0) return;
    set_context("R = rho * l / S", labels[target]);
    double R, rho, l, S;
    switch (target) {
        case 0:
            if (ask_double("rho[ohm*m]? ", &rho) && ask_double("l[m]? ", &l) && ask_double("S[m^2]? ", &S) && S != 0) {
                show_result("R", rho * l / S, "ohm");
                return;
            }
            break;
        case 1:
            if (ask_double("R[ohm]? ", &R) && ask_double("l[m]? ", &l) && ask_double("S[m^2]? ", &S) && l != 0) {
                show_result("rho", R * S / l, "ohm*m");
                return;
            }
            break;
        case 2:
            if (ask_double("R[ohm]? ", &R) && ask_double("rho[ohm*m]? ", &rho) && ask_double("S[m^2]? ", &S) && rho != 0) {
                show_result("l", R * S / rho, "m");
                return;
            }
            break;
        case 3:
            if (ask_double("R[ohm]? ", &R) && ask_double("rho[ohm*m]? ", &rho) && ask_double("l[m]? ", &l) && R != 0) {
                show_result("S", rho * l / R, "m^2");
                return;
            }
            break;
    }
    show_error();
}

static void power_block(void) {
    static const char *labels[] = {"P", "I", "U", "R"};
    int target = menu("P=I*U=I^2R=U^2/R", labels, 4);
    if (target < 0) return;
    set_context("P = I*U = I^2*R = U^2/R", labels[target]);
    double P, I, U, R;
    switch (target) {
        case 0:
            if (ask_double("I[A]? ", &I) && ask_double("U[V]? ", &U)) {
                show_result("P", I * U, "W");
                return;
            }
            if (ask_double("I[A]? ", &I) && ask_double("R[ohm]? ", &R)) {
                show_result("P", I * I * R, "W");
                return;
            }
            if (ask_double("U[V]? ", &U) && ask_double("R[ohm]? ", &R) && R != 0) {
                show_result("P", U * U / R, "W");
                return;
            }
            break;
        case 1:
            if (ask_double("P[W]? ", &P) && ask_double("U[V]? ", &U) && U != 0) {
                show_result("I", P / U, "A");
                return;
            }
            if (ask_double("P[W]? ", &P) && ask_double("R[ohm]? ", &R) && R != 0) {
                show_result("I", sqrt(P / R), "A");
                return;
            }
            if (ask_double("U[V]? ", &U) && ask_double("R[ohm]? ", &R) && R != 0) {
                show_result("I", U / R, "A");
                return;
            }
            break;
        case 2:
            if (ask_double("P[W]? ", &P) && ask_double("I[A]? ", &I) && I != 0) {
                show_result("U", P / I, "V");
                return;
            }
            if (ask_double("P[W]? ", &P) && ask_double("R[ohm]? ", &R)) {
                show_result("U", sqrt(P * R), "V");
                return;
            }
            if (ask_double("I[A]? ", &I) && ask_double("R[ohm]? ", &R)) {
                show_result("U", I * R, "V");
                return;
            }
            break;
        case 3:
            if (ask_double("U[V]? ", &U) && ask_double("I[A]? ", &I) && I != 0) {
                show_result("R", U / I, "ohm");
                return;
            }
            if (ask_double("U[V]? ", &U) && ask_double("P[W]? ", &P) && P != 0) {
                show_result("R", U * U / P, "ohm");
                return;
            }
            if (ask_double("P[W]? ", &P) && ask_double("I[A]? ", &I) && I != 0) {
                show_result("R", P / (I * I), "ohm");
                return;
            }
            break;
    }
    show_error();
}

static void charge_block(void) {
    static const char *labels[] = {"Q", "I", "t"};
    int target = menu("Q=I*t", labels, 3);
    if (target < 0) return;
    set_context("Q = I * t", labels[target]);
    double Q, I, t;
    switch (target) {
        case 0:
            if (ask_double("I[A]? ", &I) && ask_double("t[s]? ", &t)) {
                show_result("Q", I * t, "C");
                return;
            }
            break;
        case 1:
            if (ask_double("Q[C]? ", &Q) && ask_double("t[s]? ", &t) && t != 0) {
                show_result("I", Q / t, "A");
                return;
            }
            break;
        case 2:
            if (ask_double("Q[C]? ", &Q) && ask_double("I[A]? ", &I) && I != 0) {
                show_result("t", Q / I, "s");
                return;
            }
            break;
    }
    show_error();
}

static void wiedemann_block(void) {
    static const char *labels[] = {"lambda_T", "gamma", "T"};
    int target = menu("lambda_T/gamma=L*T", labels, 3);
    if (target < 0) return;
    set_context("lambda_T/gamma = L*T", labels[target]);
    double lambda_T, gamma, T;
    switch (target) {
        case 0:
            if (ask_double("gamma[S/m]? ", &gamma) && ask_double("T[K]? ", &T)) {
                show_result("lambda_T", LORENZ * T * gamma, "W/mK");
                return;
            }
            break;
        case 1:
            if (ask_double("lambda_T[W/mK]? ", &lambda_T) && ask_double("T[K]? ", &T) && T != 0) {
                show_result("gamma", lambda_T / (LORENZ * T), "S/m");
                return;
            }
            break;
        case 2:
            if (ask_double("lambda_T[W/mK]? ", &lambda_T) && ask_double("gamma[S/m]? ", &gamma) && gamma != 0) {
                show_result("T", lambda_T / (LORENZ * gamma), "K");
                return;
            }
            break;
    }
    show_error();
}

static void superconductor_block(void) {
    static const char *labels[] = {"Hkr", "Hkr0", "Tkr", "Tkr0"};
    int target = menu("Hkr=Hkr0(1-(T/T0)^2)", labels, 4);
    if (target < 0) return;
    set_context("Hkr=Hkr0*(1-(T/T0)^2)", labels[target]);
    double H, H0, T, T0;
    switch (target) {
        case 0:
            if (ask_double("Hkr0[A/m]? ", &H0) && ask_double("Tkr[K]? ", &T) && ask_double("Tkr0[K]? ", &T0) && T0 != 0) {
                double ratio = (T / T0) * (T / T0);
                show_result("Hkr", H0 * (1 - ratio), "A/m");
                return;
            }
            break;
        case 1:
            if (ask_double("Hkr[A/m]? ", &H) && ask_double("Tkr[K]? ", &T) && ask_double("Tkr0[K]? ", &T0)) {
                double ratio = (T / T0) * (T / T0);
                if (1 - ratio != 0) {
                    show_result("Hkr0", H / (1 - ratio), "A/m");
                    return;
                }
            }
            break;
        case 2:
            if (ask_double("Hkr[A/m]? ", &H) && ask_double("Hkr0[A/m]? ", &H0) && ask_double("Tkr0[K]? ", &T0) && H0 != 0) {
                double r = 1 - H / H0;
                if (r >= 0) {
                    show_result("Tkr", T0 * sqrt(r), "K");
                    return;
                }
            }
            break;
        case 3:
            if (ask_double("Hkr[A/m]? ", &H) && ask_double("Hkr0[A/m]? ", &H0) && ask_double("Tkr[K]? ", &T) && H0 != 0) {
                double r = 1 - H / H0;
                if (r > 0) {
                    show_result("Tkr0", T / sqrt(r), "K");
                    return;
                }
            }
            break;
    }
    show_error();
}

static void H_field_block(void) {
    static const char *labels[] = {"H", "N", "I", "l"};
    int target = menu("H=N*I/l", labels, 4);
    if (target < 0) return;
    set_context("H = N * I / l", labels[target]);
    double H, N, I, l;
    switch (target) {
        case 0:
            if (ask_double("N? ", &N) && ask_double("I[A]? ", &I) && ask_double("l[m]? ", &l) && l != 0) {
                show_result("H", N * I / l, "A/m");
                return;
            }
            break;
        case 1:
            if (ask_double("H[A/m]? ", &H) && ask_double("I[A]? ", &I) && ask_double("l[m]? ", &l) && I != 0) {
                show_result("N", H * l / I, "");
                return;
            }
            break;
        case 2:
            if (ask_double("H[A/m]? ", &H) && ask_double("N? ", &N) && ask_double("l[m]? ", &l) && N != 0) {
                show_result("I", H * l / N, "A");
                return;
            }
            break;
        case 3:
            if (ask_double("H[A/m]? ", &H) && ask_double("N? ", &N) && ask_double("I[A]? ", &I) && H != 0) {
                show_result("l", N * I / H, "m");
                return;
            }
            break;
    }
    show_error();
}

static void induction_block(void) {
    static const char *labels[] = {"B", "mu_r", "H"};
    int target = menu("B=mu0*mu_r*H", labels, 3);
    if (target < 0) return;
    set_context("B = mu0 * mu_r * H", labels[target]);
    double B, mu_r, H;
    switch (target) {
        case 0:
            if (ask_double("mu_r? ", &mu_r) && ask_double("H[A/m]? ", &H)) {
                show_result("B", MU0 * mu_r * H, "T");
                return;
            }
            break;
        case 1:
            if (ask_double("B[T]? ", &B) && ask_double("H[A/m]? ", &H) && H != 0) {
                show_result("mu_r", B / (MU0 * H), "");
                return;
            }
            break;
        case 2:
            if (ask_double("B[T]? ", &B) && ask_double("mu_r? ", &mu_r) && mu_r != 0) {
                show_result("H", B / (MU0 * mu_r), "A/m");
                return;
            }
            break;
    }
    show_error();
}

static void gumlich_block(void) {
    static const char *labels[] = {"mu_r", "Br", "Hc"};
    int target = menu("mu_r~Br/(2mu0Hc)", labels, 3);
    if (target < 0) return;
    set_context("mu_r ~= Br/(2*mu0*Hc)", labels[target]);
    double mu_r, Br, Hc;
    switch (target) {
        case 0:
            if (ask_double("Br[T]? ", &Br) && ask_double("Hc[A/m]? ", &Hc) && Hc != 0) {
                show_result("mu_r", Br / (2 * MU0 * Hc), "");
                return;
            }
            break;
        case 1:
            if (ask_double("mu_r? ", &mu_r) && ask_double("Hc[A/m]? ", &Hc)) {
                show_result("Br", 2 * MU0 * mu_r * Hc, "T");
                return;
            }
            break;
        case 2:
            if (ask_double("mu_r? ", &mu_r) && ask_double("Br[T]? ", &Br) && mu_r != 0) {
                show_result("Hc", Br / (2 * MU0 * mu_r), "A/m");
                return;
            }
            break;
    }
    show_error();
}

static void electromagnet_force_block(void) {
    static const char *labels[] = {"F", "M_r", "S"};
    int target = menu("F=0.5mu0*M^2*S", labels, 3);
    if (target < 0) return;
    set_context("F = 0.5*mu0*M^2*S", labels[target]);
    double F, M, S;
    switch (target) {
        case 0:
            if (ask_double("M_r[A/m]? ", &M) && ask_double("S[m^2]? ", &S)) {
                show_result("F", 0.5 * MU0 * M * M * S, "N");
                return;
            }
            break;
        case 1:
            if (ask_double("F[N]? ", &F) && ask_double("S[m^2]? ", &S) && S != 0) {
                double val = 2 * F / (MU0 * S);
                if (val >= 0) {
                    show_result("M_r", sqrt(val), "A/m");
                    return;
                }
            }
            break;
        case 2:
            if (ask_double("F[N]? ", &F) && ask_double("M_r[A/m]? ", &M) && M != 0) {
                show_result("S", 2 * F / (MU0 * M * M), "m^2");
                return;
            }
            break;
    }
    show_error();
}

static void remanence_block(void) {
    static const char *labels[] = {"Br", "M_r"};
    int target = menu("Br=mu0*M_r", labels, 2);
    if (target < 0) return;
    set_context("Br = mu0 * M_r", labels[target]);
    double Br, M;
    if (target == 0) {
        if (ask_double("M_r[A/m]? ", &M)) {
            show_result("Br", MU0 * M, "T");
            return;
        }
    } else {
        if (ask_double("Br[T]? ", &Br)) {
            show_result("M_r", Br / MU0, "A/m");
            return;
        }
    }
    show_error();
}

static void capacitance_block(void) {
    static const char *labels[] = {"C", "eps_r", "S", "d"};
    int target = menu("C=eps0*er*S/d", labels, 4);
    if (target < 0) return;
    set_context("C = eps0*eps_r*S/d", labels[target]);
    double C, eps_r, S, d;
    switch (target) {
        case 0:
            if (ask_double("eps_r? ", &eps_r) && ask_double("S[m^2]? ", &S) && ask_double("d[m]? ", &d) && d != 0) {
                show_result("C", EPS0 * eps_r * S / d, "F");
                return;
            }
            break;
        case 1:
            if (ask_double("C[F]? ", &C) && ask_double("S[m^2]? ", &S) && ask_double("d[m]? ", &d) && S != 0) {
                show_result("eps_r", C * d / (EPS0 * S), "");
                return;
            }
            break;
        case 2:
            if (ask_double("C[F]? ", &C) && ask_double("eps_r? ", &eps_r) && ask_double("d[m]? ", &d) && eps_r != 0) {
                show_result("S", C * d / (EPS0 * eps_r), "m^2");
                return;
            }
            break;
        case 3:
            if (ask_double("C[F]? ", &C) && ask_double("eps_r? ", &eps_r) && ask_double("S[m^2]? ", &S) && C != 0) {
                show_result("d", EPS0 * eps_r * S / C, "m");
                return;
            }
            break;
    }
    show_error();
}

static void capacitor_charge_block(void) {
    static const char *labels[] = {"Q", "C", "U"};
    int target = menu("Q=C*U", labels, 3);
    if (target < 0) return;
    set_context("Q = C * U", labels[target]);
    double Q, C, U;
    switch (target) {
        case 0:
            if (ask_double("C[F]? ", &C) && ask_double("U[V]? ", &U)) {
                show_result("Q", C * U, "C");
                return;
            }
            break;
        case 1:
            if (ask_double("Q[C]? ", &Q) && ask_double("U[V]? ", &U) && U != 0) {
                show_result("C", Q / U, "F");
                return;
            }
            break;
        case 2:
            if (ask_double("Q[C]? ", &Q) && ask_double("C[F]? ", &C) && C != 0) {
                show_result("U", Q / C, "V");
                return;
            }
            break;
    }
    show_error();
}

static void heat_transfer_block(void) {
    static const char *labels[] = {"Q", "lambda_T", "S", "h", "deltaT"};
    int target = menu("Q=lambda*(S/h)*dT", labels, 5);
    if (target < 0) return;
    set_context("Q = lambda_T*(S/h)*dT", labels[target]);
    double Q, lam, S, h, dT;
    switch (target) {
        case 0:
            if (ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("h[m]? ", &h) && ask_double("dT[K]? ", &dT) && h != 0) {
                show_result("Q", lam * S * dT / h, "W");
                return;
            }
            break;
        case 1:
            if (ask_double("Q[W]? ", &Q) && ask_double("S[m^2]? ", &S) && ask_double("h[m]? ", &h) && ask_double("dT[K]? ", &dT) && S != 0 && dT != 0) {
                show_result("lambda_T", Q * h / (S * dT), "W/mK");
                return;
            }
            break;
        case 2:
            if (ask_double("Q[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("h[m]? ", &h) && ask_double("dT[K]? ", &dT) && lam != 0 && dT != 0) {
                show_result("S", Q * h / (lam * dT), "m^2");
                return;
            }
            break;
        case 3:
            if (ask_double("Q[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("dT[K]? ", &dT) && Q != 0) {
                show_result("h", lam * S * dT / Q, "m");
                return;
            }
            break;
        case 4:
            if (ask_double("Q[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("h[m]? ", &h) && lam != 0 && S != 0) {
                show_result("deltaT", Q * h / (lam * S), "K");
                return;
            }
            break;
    }
    show_error();
}

static void em_wave_velocity_block(void) {
    static const char *labels[] = {"v", "eps_r", "mu_r"};
    int target = menu("v=1/sqrt(eps0*er*mu0*mr)", labels, 3);
    if (target < 0) return;
    set_context("v=1/sqrt(eps0*er*mu0*mr)", labels[target]);
    double v, eps_r, mu_r;
    switch (target) {
        case 0:
            if (ask_double("eps_r? ", &eps_r) && ask_double("mu_r? ", &mu_r) && eps_r != 0 && mu_r != 0) {
                show_result("v", 1.0 / sqrt(EPS0 * eps_r * MU0 * mu_r), "m/s");
                return;
            }
            break;
        case 1:
            if (ask_double("v[m/s]? ", &v) && ask_double("mu_r? ", &mu_r) && v != 0 && mu_r != 0) {
                show_result("eps_r", 1.0 / (v * v * MU0 * mu_r * EPS0), "");
                return;
            }
            break;
        case 2:
            if (ask_double("v[m/s]? ", &v) && ask_double("eps_r? ", &eps_r) && v != 0 && eps_r != 0) {
                show_result("mu_r", 1.0 / (v * v * EPS0 * MU0 * eps_r), "");
                return;
            }
            break;
    }
    show_error();
}

static void wavelength_block(void) {
    static const char *labels[] = {"lambda", "v", "f"};
    int target = menu("lambda=v/f", labels, 3);
    if (target < 0) return;
    set_context("lambda = v / f", labels[target]);
    double lambda, v, f;
    switch (target) {
        case 0:
            if (ask_double("v[m/s]? ", &v) && ask_double("f[Hz]? ", &f) && f != 0) {
                show_result("lambda", v / f, "m");
                return;
            }
            break;
        case 1:
            if (ask_double("lambda[m]? ", &lambda) && ask_double("f[Hz]? ", &f)) {
                show_result("v", lambda * f, "m/s");
                return;
            }
            break;
        case 2:
            if (ask_double("lambda[m]? ", &lambda) && ask_double("v[m/s]? ", &v) && lambda != 0) {
                show_result("f", v / lambda, "Hz");
                return;
            }
            break;
    }
    show_error();
}

static void capacitor_energy_block(void) {
    static const char *labels[] = {"E", "C", "U"};
    int target = menu("E=0.5*C*U^2", labels, 3);
    if (target < 0) return;
    set_context("E = 0.5 * C * U^2", labels[target]);
    double E, C, U;
    switch (target) {
        case 0:
            if (ask_double("C[F]? ", &C) && ask_double("U[V]? ", &U)) {
                show_result("E", 0.5 * C * U * U, "J");
                return;
            }
            break;
        case 1:
            if (ask_double("E[J]? ", &E) && ask_double("U[V]? ", &U) && U != 0) {
                show_result("C", 2 * E / (U * U), "F");
                return;
            }
            break;
        case 2:
            if (ask_double("E[J]? ", &E) && ask_double("C[F]? ", &C) && C != 0) {
                show_result("U", sqrt(2 * E / C), "V");
                return;
            }
            break;
    }
    show_error();
}

static void electric_field_block(void) {
    static const char *labels[] = {"E", "U", "d"};
    int target = menu("E=U/d", labels, 3);
    if (target < 0) return;
    set_context("E = U / d", labels[target]);
    double E, U, d;
    switch (target) {
        case 0:
            if (ask_double("U[V]? ", &U) && ask_double("d[m]? ", &d) && d != 0) {
                show_result("E", U / d, "V/m");
                return;
            }
            break;
        case 1:
            if (ask_double("E[V/m]? ", &E) && ask_double("d[m]? ", &d)) {
                show_result("U", E * d, "V");
                return;
            }
            break;
        case 2:
            if (ask_double("U[V]? ", &U) && ask_double("E[V/m]? ", &E) && E != 0) {
                show_result("d", U / E, "m");
                return;
            }
            break;
    }
    show_error();
}

static void electric_flux_block(void) {
    static const char *labels[] = {"D", "eps_r", "E"};
    int target = menu("D=eps0*er*E", labels, 3);
    if (target < 0) return;
    set_context("D = eps0*eps_r*E", labels[target]);
    double D, eps_r, E;
    switch (target) {
        case 0:
            if (ask_double("eps_r? ", &eps_r) && ask_double("E[V/m]? ", &E)) {
                show_result("D", EPS0 * eps_r * E, "C/m^2");
                return;
            }
            break;
        case 1:
            if (ask_double("D[C/m^2]? ", &D) && ask_double("E[V/m]? ", &E) && E != 0) {
                show_result("eps_r", D / (EPS0 * E), "");
                return;
            }
            break;
        case 2:
            if (ask_double("D[C/m^2]? ", &D) && ask_double("eps_r? ", &eps_r) && eps_r != 0) {
                show_result("E", D / (EPS0 * eps_r), "V/m");
                return;
            }
            break;
    }
    show_error();
}

static void thermal_flux_block(void) {
    static const char *labels[] = {"Q_T", "lambda_T", "S", "d", "deltaTheta"};
    int target = menu("Q_T=lambda*(S/d)*dTh", labels, 5);
    if (target < 0) return;
    set_context("Q_T=lambda_T*(S/d)*dTheta", labels[target]);
    double Q, lam, S, d, dth;
    switch (target) {
        case 0:
            if (ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("d[m]? ", &d) && ask_double("dTheta[K]? ", &dth) && d != 0) {
                show_result("Q_T", lam * S * dth / d, "W");
                return;
            }
            break;
        case 1:
            if (ask_double("Q_T[W]? ", &Q) && ask_double("S[m^2]? ", &S) && ask_double("d[m]? ", &d) && ask_double("dTheta[K]? ", &dth) && S != 0 && dth != 0) {
                show_result("lambda_T", Q * d / (S * dth), "W/mK");
                return;
            }
            break;
        case 2:
            if (ask_double("Q_T[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("d[m]? ", &d) && ask_double("dTheta[K]? ", &dth) && lam != 0 && dth != 0) {
                show_result("S", Q * d / (lam * dth), "m^2");
                return;
            }
            break;
        case 3:
            if (ask_double("Q_T[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("dTheta[K]? ", &dth) && Q != 0) {
                show_result("d", lam * S * dth / Q, "m");
                return;
            }
            break;
        case 4:
            if (ask_double("Q_T[W]? ", &Q) && ask_double("lambda[W/mK]? ", &lam) && ask_double("S[m^2]? ", &S) && ask_double("d[m]? ", &d) && lam != 0 && S != 0) {
                show_result("deltaTheta", Q * d / (lam * S), "K");
                return;
            }
            break;
    }
    show_error();
}

static void intrinsic_conductivity_block(void) {
    static const char *labels[] = {"gamma", "n_i", "u_n", "u_p"};
    int target = menu("gamma=e*n_i(u_n+u_p)", labels, 4);
    if (target < 0) return;
    set_context("gamma = e*n_i*(u_n+u_p)", labels[target]);
    double gamma, n_i, u_n, u_p;
    switch (target) {
        case 0:
            if (ask_double("n_i[1/m^3]? ", &n_i) && ask_double("u_n[m^2/Vs]? ", &u_n) && ask_double("u_p[m^2/Vs]? ", &u_p)) {
                show_result("gamma", E_CHARGE * n_i * (u_n + u_p), "S/m");
                return;
            }
            break;
        case 1:
            if (ask_double("gamma[S/m]? ", &gamma) && ask_double("u_n[m^2/Vs]? ", &u_n) && ask_double("u_p[m^2/Vs]? ", &u_p) && (u_n + u_p) != 0) {
                show_result("n_i", gamma / (E_CHARGE * (u_n + u_p)), "1/m^3");
                return;
            }
            break;
        case 2:
            if (ask_double("gamma[S/m]? ", &gamma) && ask_double("n_i[1/m^3]? ", &n_i) && ask_double("u_p[m^2/Vs]? ", &u_p) && n_i != 0) {
                show_result("u_n", gamma / (E_CHARGE * n_i) - u_p, "m^2/Vs");
                return;
            }
            break;
        case 3:
            if (ask_double("gamma[S/m]? ", &gamma) && ask_double("n_i[1/m^3]? ", &n_i) && ask_double("u_n[m^2/Vs]? ", &u_n) && n_i != 0) {
                show_result("u_p", gamma / (E_CHARGE * n_i) - u_n, "m^2/Vs");
                return;
            }
            break;
    }
    show_error();
}

static void einstein_block(void) {
    static const char *labels[] = {"u", "D", "T"};
    int target = menu("u/D=e/(kT)", labels, 3);
    if (target < 0) return;
    set_context("u / D = e / (k*T)", labels[target]);
    double u, D, T;
    switch (target) {
        case 0:
            if (ask_double("D[m^2/s]? ", &D) && ask_double("T[K]? ", &T) && T != 0) {
                show_result("u", E_CHARGE * D / (K_B * T), "m^2/Vs");
                return;
            }
            break;
        case 1:
            if (ask_double("u[m^2/Vs]? ", &u) && ask_double("T[K]? ", &T)) {
                show_result("D", u * K_B * T / E_CHARGE, "m^2/s");
                return;
            }
            break;
        case 2:
            if (ask_double("u[m^2/Vs]? ", &u) && ask_double("D[m^2/s]? ", &D) && u != 0) {
                show_result("T", E_CHARGE * D / (K_B * u), "K");
                return;
            }
            break;
    }
    show_error();
}

static void potential_block(void) {
    static const char *labels[] = {"n_n", "V", "T"};
    int target = menu("n_n=n_i*exp(eV/kT)", labels, 3);
    if (target < 0) return;
    set_context("n_n = n_i * exp(eV/kT)", labels[target]);
    double n_n, n_i, V, T;
    switch (target) {
        case 0:
            if (ask_double("n_i[1/m^3]? ", &n_i) && ask_double("V[V]? ", &V) && ask_double("T[K]? ", &T) && T != 0) {
                show_result("n_n", n_i * exp(E_CHARGE * V / (K_B * T)), "1/m^3");
                return;
            }
            break;
        case 1:
            if (ask_double("n_n[1/m^3]? ", &n_n) && ask_double("n_i[1/m^3]? ", &n_i) && ask_double("T[K]? ", &T) && n_i != 0 && T != 0) {
                double ratio = n_n / n_i;
                if (ratio > 0) {
                    show_result("V", log(ratio) * K_B * T / E_CHARGE, "V");
                    return;
                }
            }
            break;
        case 2:
            if (ask_double("n_n[1/m^3]? ", &n_n) && ask_double("n_i[1/m^3]? ", &n_i) && ask_double("V[V]? ", &V) && n_i != 0 && V != 0) {
                double ratio = n_n / n_i;
                if (ratio > 0) {
                    show_result("T", E_CHARGE * V / (K_B * log(ratio)), "K");
                    return;
                }
            }
            break;
    }
    show_error();
}

static void hall_block(void) {
    static const char *labels[] = {"U_H", "R_H", "I", "B", "h"};
    int target = menu("U_H=R_H*I*B/h", labels, 5);
    if (target < 0) return;
    set_context("U_H = R_H * I * B / h", labels[target]);
    double UH, RH, I, B, h;
    switch (target) {
        case 0:
            if (ask_double("R_H[m^3/C]? ", &RH) && ask_double("I[A]? ", &I) && ask_double("B[T]? ", &B) && ask_double("h[m]? ", &h) && h != 0) {
                show_result("U_H", RH * I * B / h, "V");
                return;
            }
            break;
        case 1:
            if (ask_double("U_H[V]? ", &UH) && ask_double("I[A]? ", &I) && ask_double("B[T]? ", &B) && ask_double("h[m]? ", &h) && I != 0 && B != 0) {
                show_result("R_H", UH * h / (I * B), "m^3/C");
                return;
            }
            break;
        case 2:
            if (ask_double("U_H[V]? ", &UH) && ask_double("R_H[m^3/C]? ", &RH) && ask_double("B[T]? ", &B) && ask_double("h[m]? ", &h) && RH != 0 && B != 0) {
                show_result("I", UH * h / (RH * B), "A");
                return;
            }
            break;
        case 3:
            if (ask_double("U_H[V]? ", &UH) && ask_double("R_H[m^3/C]? ", &RH) && ask_double("I[A]? ", &I) && ask_double("h[m]? ", &h) && RH != 0 && I != 0) {
                show_result("B", UH * h / (RH * I), "T");
                return;
            }
            break;
        case 4:
            if (ask_double("U_H[V]? ", &UH) && ask_double("R_H[m^3/C]? ", &RH) && ask_double("I[A]? ", &I) && ask_double("B[T]? ", &B) && UH != 0) {
                show_result("h", RH * I * B / UH, "m");
                return;
            }
            break;
    }
    show_error();
}

static void seebeck_block(void) {
    static const char *labels[] = {"U_s", "DeltaS", "DeltaT"};
    int target = menu("U_s=DeltaS*DeltaT", labels, 3);
    if (target < 0) return;
    set_context("U_s = DeltaS * DeltaT", labels[target]);
    double U, Sdiff, dT;
    switch (target) {
        case 0:
            if (ask_double("DeltaS[V/K]? ", &Sdiff) && ask_double("DeltaT[K]? ", &dT)) {
                show_result("U_s", Sdiff * dT, "V");
                return;
            }
            break;
        case 1:
            if (ask_double("U_s[V]? ", &U) && ask_double("DeltaT[K]? ", &dT) && dT != 0) {
                show_result("DeltaS", U / dT, "V/K");
                return;
            }
            break;
        case 2:
            if (ask_double("U_s[V]? ", &U) && ask_double("DeltaS[V/K]? ", &Sdiff) && Sdiff != 0) {
                show_result("DeltaT", U / Sdiff, "K");
                return;
            }
            break;
    }
    show_error();
}

static void ntc_resistance_block(void) {
    static const char *labels[] = {"R", "A", "b", "T"};
    int target = menu("R=A*exp(b/T)", labels, 4);
    if (target < 0) return;
    set_context("R = A * exp(b / T)", labels[target]);
    double R, A, b, T;
    switch (target) {
        case 0:
            if (ask_double("A[ohm]? ", &A) && ask_double("b[K]? ", &b) && ask_double("T[K]? ", &T)) {
                show_result("R", A * exp(b / T), "ohm");
                return;
            }
            break;
        case 1:
            if (ask_double("R[ohm]? ", &R) && ask_double("b[K]? ", &b) && ask_double("T[K]? ", &T)) {
                show_result("A", R / exp(b / T), "ohm");
                return;
            }
            break;
        case 2:
            if (ask_double("R[ohm]? ", &R) && ask_double("A[ohm]? ", &A) && ask_double("T[K]? ", &T) && A != 0 && T != 0) {
                double ratio = R / A;
                if (ratio > 0) {
                    show_result("b", T * log(ratio), "K");
                    return;
                }
            }
            break;
        case 3:
            if (ask_double("R[ohm]? ", &R) && ask_double("A[ohm]? ", &A) && ask_double("b[K]? ", &b) && A != 0) {
                double ratio = R / A;
                if (ratio > 0 && b != 0) {
                    show_result("T", b / log(ratio), "K");
                    return;
                }
            }
            break;
    }
    show_error();
}

static void ntc_alpha_block(void) {
    static const char *labels[] = {"alpha_R", "b", "T"};
    int target = menu("alpha_R=-b/T^2", labels, 3);
    if (target < 0) return;
    set_context("alpha_R = -b / T^2", labels[target]);
    double alpha, b, T;
    switch (target) {
        case 0:
            if (ask_double("b[K]? ", &b) && ask_double("T[K]? ", &T) && T != 0) {
                show_result("alpha_R", -b / (T * T), "1/K");
                return;
            }
            break;
        case 1:
            if (ask_double("alpha_R[1/K]? ", &alpha) && ask_double("T[K]? ", &T)) {
                show_result("b", -alpha * T * T, "K");
                return;
            }
            break;
        case 2:
            if (ask_double("alpha_R[1/K]? ", &alpha) && ask_double("b[K]? ", &b) && alpha != 0) {
                double val = -b / alpha;
                if (val > 0) {
                    show_result("T", sqrt(val), "K");
                    return;
                }
            }
            break;
    }
    show_error();
}

static void conductive_menu(void) {
    const char *opts[] = {
        "Force / accel",
        "Current density",
        "Mobility u=v/E",
        "Temp coeff RT",
        "Resistance R=rl/S",
        "Power P/I/U/R",
        "Charge Q=I*t",
        "Wiedemann-Franz",
        "Superconductor",
    };
    while (1) {
        int choice = menu("Conductive", opts, sizeof(opts) / sizeof(opts[0]));
        if (choice < 0) return;
        switch (choice) {
            case 0: force_block(); break;
            case 1: current_density_block(); break;
            case 2: mobility_block(); break;
            case 3: tcr_block(); break;
            case 4: resistance_block(); break;
            case 5: power_block(); break;
            case 6: charge_block(); break;
            case 7: wiedemann_block(); break;
            case 8: superconductor_block(); break;
        }
    }
}

static void magnetic_menu(void) {
    const char *opts[] = {
        "Field H=N*I/l",
        "Induction B=muH",
        "Gumlich mu_r~",
        "Electromagnet F",
        "Remanence Br",
    };
    while (1) {
        int choice = menu("Magnetic", opts, sizeof(opts) / sizeof(opts[0]));
        if (choice < 0) return;
        switch (choice) {
            case 0: H_field_block(); break;
            case 1: induction_block(); break;
            case 2: gumlich_block(); break;
            case 3: electromagnet_force_block(); break;
            case 4: remanence_block(); break;
        }
    }
}

static void dielectric_menu(void) {
    const char *opts[] = {
        "Capacitance C",
        "Cap charge Q=C*U",
        "Heat Q=lambda*S/h",
        "EM wave v",
        "Wavelength",
        "Cap energy E",
        "Electric field",
        "Flux density D",
        "Thermal flux",
    };
    while (1) {
        int choice = menu("Dielectric", opts, sizeof(opts) / sizeof(opts[0]));
        if (choice < 0) return;
        switch (choice) {
            case 0: capacitance_block(); break;
            case 1: capacitor_charge_block(); break;
            case 2: heat_transfer_block(); break;
            case 3: em_wave_velocity_block(); break;
            case 4: wavelength_block(); break;
            case 5: capacitor_energy_block(); break;
            case 6: electric_field_block(); break;
            case 7: electric_flux_block(); break;
            case 8: thermal_flux_block(); break;
        }
    }
}

static void semiconductor_menu(void) {
    const char *opts[] = {
        "Intrinsic cond",
        "Einstein u/D",
        "Potential n_i/n",
        "Hall voltage",
        "Seebeck",
        "NTC resist",
        "NTC alpha",
    };
    while (1) {
        int choice = menu("Semicond", opts, sizeof(opts) / sizeof(opts[0]));
        if (choice < 0) return;
        switch (choice) {
            case 0: intrinsic_conductivity_block(); break;
            case 1: einstein_block(); break;
            case 2: potential_block(); break;
            case 3: hall_block(); break;
            case 4: seebeck_block(); break;
            case 5: ntc_resistance_block(); break;
            case 6: ntc_alpha_block(); break;
        }
    }
}

int main(void) {
    const char *opts[] = {"Conductive", "Magnetic", "Dielectric", "Semiconductors"};
    while (1) {
        int cat = menu("FME solver (Enter=exit)", opts, 4);
        if (cat < 0) break;
        switch (cat) {
            case 0: conductive_menu(); break;
            case 1: magnetic_menu(); break;
            case 2: dielectric_menu(); break;
            case 3: semiconductor_menu(); break;
        }
    }
    os_ClrHome();
    os_PutStrFull("meow!");
    os_NewLine();
    os_PutStrFull("Press key...");
    wait_key();
    return 0;
}

