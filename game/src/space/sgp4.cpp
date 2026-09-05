#include "space/sgp4.hpp"

#include <cmath>

#include <glm/gtc/constants.hpp>

#include "components/orbital_elements_component.hpp"

namespace WingsOfSteel
{

namespace
{

    // Appears throughout as the exponent relating mean motion to semi-major axis, and written out
    // rather than folded into the calls so that the division happens once and identically at each
    // use, which is what lets the result be compared against the reference exactly.
    constexpr double kTwoThirds = 2.0 / 3.0;

    // The period, in minutes, at or above which the element set belongs to SDP4 instead. Works out
    // at a semi-major axis of about 12250 km, so a circular orbit above roughly 5900 km - and any
    // orbit eccentric enough to take that long whatever its perigee.
    constexpr double kDeepSpacePeriodMinutes = 225.0;

    // Perigee below this altitude in km and the drag model drops its higher order terms.
    constexpr double kSimplifiedDragPerigee = 220.0;

    // Guards the division by (1 + cos i) in xlcof against an exactly retrograde orbit. The value is
    // the reference's, and it is a threshold on the divisor rather than on the inclination.
    constexpr double kRetrogradeSingularityGuard = 1.5e-12;

    // Revolutions per day to radians per minute, as a divisor: the reference divides by this
    // quantity rather than multiplying by its reciprocal, and the last bit of the mean motion is
    // worth keeping identical.
    const double kRevsPerDayToRadsPerMinute = 1440.0 / glm::two_pi<double>();

} // namespace

SGP4ElementSet SGP4Initialise(const SGP4Elements& elements)
{
    SGP4ElementSet elementSet;

    // The step reads the mean elements themselves at every call, so they travel with the
    // coefficients rather than being looked up separately.
    elementSet.bstar = elements.bstar;
    elementSet.ecco = elements.ecco;
    elementSet.inclo = elements.inclo;
    elementSet.nodeo = elements.nodeo;
    elementSet.argpo = elements.argpo;
    elementSet.mo = elements.mo;

    // The atmospheric model's boundary: the drag term is fitted between 78 km and 120 km altitude,
    // expressed in earth radii.
    const double ss = 78.0 / kSGP4EarthRadius + 1.0;
    const double qzms2ttemp = (120.0 - 78.0) / kSGP4EarthRadius;
    const double qzms2t = qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;

    // --- The preamble both branches share (the reference's initl) ---

    const double eccsq = elements.ecco * elements.ecco;
    const double omeosq = 1.0 - eccsq;
    const double rteosq = std::sqrt(omeosq);
    const double cosio = std::cos(elements.inclo);
    const double cosio2 = cosio * cosio;

    // Un-Kozai the mean motion. A TLE states the mean motion of Kozai's theory, which carries a
    // secular J2 term SGP4 accounts for separately; using it unaltered double-counts that term.
    // Two passes of the same correction, the second refining the semi-major axis the first found.
    const double ak = std::pow(kSGP4Xke / elements.noKozai, kTwoThirds);
    const double d1 = 0.75 * kSGP4J2 * (3.0 * cosio2 - 1.0) / (rteosq * omeosq);
    double del = d1 / (ak * ak);
    const double adel = ak * (1.0 - del * del - del * (1.0 / 3.0 + 134.0 * del * del / 81.0));
    del = d1 / (adel * adel);
    const double no_unkozai = elements.noKozai / (1.0 + del);

    const double ao = std::pow(kSGP4Xke / no_unkozai, kTwoThirds);
    const double sinio = std::sin(elements.inclo);
    const double po = ao * omeosq;
    const double con42 = 1.0 - 5.0 * cosio2;
    const double con41 = -con42 - cosio2 - cosio2;
    const double posq = po * po;
    const double rp = ao * (1.0 - elements.ecco);

    // Deep space is a different algorithm rather than a longer version of this one, and it is not
    // implemented. Reporting it and stopping is the honest answer: the near-earth coefficients
    // below are computed for these orbits too by the reference, but they are only half of what
    // SDP4 then steps with, and half a coefficient block is more dangerous than none because it
    // looks usable. Every derived field is therefore left at zero.
    if (glm::two_pi<double>() / no_unkozai >= kDeepSpacePeriodMinutes)
    {
        elementSet.method = SGP4Method::DeepSpace;
        return elementSet;
    }

    elementSet.no_unkozai = no_unkozai;
    elementSet.con41 = con41;

    // The reference's own guard. It is an or, not an and, so it holds for anything short of both
    // an eccentricity past one and a negative mean motion; failing it leaves the coefficients zero.
    if ((omeosq >= 0.0) || (no_unkozai >= 0.0))
    {
        elementSet.simplifiedDrag = rp < (kSimplifiedDragPerigee / kSGP4EarthRadius + 1.0);

        double sfour = ss;
        double qzms24 = qzms2t;
        const double perige = (rp - 1.0) * kSGP4EarthRadius;

        // Below 156 km the fitted atmospheric boundary is moved down to meet the orbit, and below
        // 98 km it stops moving and sits at 20 km.
        if (perige < 156.0)
        {
            sfour = perige - 78.0;
            if (perige < 98.0)
            {
                sfour = 20.0;
            }
            const double qzms24temp = (120.0 - sfour) / kSGP4EarthRadius;
            qzms24 = qzms24temp * qzms24temp * qzms24temp * qzms24temp;
            sfour = sfour / kSGP4EarthRadius + 1.0;
        }

        const double pinvsq = 1.0 / posq;
        const double tsi = 1.0 / (ao - sfour);
        elementSet.eta = ao * elements.ecco * tsi;
        const double etasq = elementSet.eta * elementSet.eta;
        const double eeta = elements.ecco * elementSet.eta;
        const double psisq = std::fabs(1.0 - etasq);
        const double coef = qzms24 * std::pow(tsi, 4.0);
        const double coef1 = coef / std::pow(psisq, 3.5);
        const double cc2 = coef1 * no_unkozai * (ao * (1.0 + 1.5 * etasq + eeta * (4.0 + etasq)) + 0.375 * kSGP4J2 * tsi / psisq * con41 * (8.0 + 3.0 * etasq * (8.0 + etasq)));
        elementSet.cc1 = elements.bstar * cc2;

        // cc3 and xmcof both divide by the eccentricity, so a near-circular orbit takes the terms
        // they feed as zero rather than as an overflow.
        double cc3 = 0.0;
        if (elements.ecco > 1.0e-4)
        {
            cc3 = -2.0 * coef * tsi * kSGP4J3OverJ2 * no_unkozai * sinio / elements.ecco;
        }

        elementSet.x1mth2 = 1.0 - cosio2;
        elementSet.cc4 = 2.0 * no_unkozai * coef1 * ao * omeosq * (elementSet.eta * (2.0 + 0.5 * etasq) + elements.ecco * (0.5 + 2.0 * etasq) - kSGP4J2 * tsi / (ao * psisq) * (-3.0 * con41 * (1.0 - 2.0 * eeta + etasq * (1.5 - 0.5 * eeta)) + 0.75 * elementSet.x1mth2 * (2.0 * etasq - eeta * (1.0 + etasq)) * std::cos(2.0 * elements.argpo)));
        elementSet.cc5 = 2.0 * coef1 * ao * omeosq * (1.0 + 2.75 * (etasq + eeta) + eeta * etasq);

        // The secular rates of the mean anomaly, argument of perigee and node under J2, J2 squared
        // and J4.
        const double cosio4 = cosio2 * cosio2;
        const double temp1 = 1.5 * kSGP4J2 * pinvsq * no_unkozai;
        const double temp2 = 0.5 * temp1 * kSGP4J2 * pinvsq;
        const double temp3 = -0.46875 * kSGP4J4 * pinvsq * pinvsq * no_unkozai;
        elementSet.mdot = no_unkozai + 0.5 * temp1 * rteosq * con41 + 0.0625 * temp2 * rteosq * (13.0 - 78.0 * cosio2 + 137.0 * cosio4);
        elementSet.argpdot = -0.5 * temp1 * con42 + 0.0625 * temp2 * (7.0 - 114.0 * cosio2 + 395.0 * cosio4) + temp3 * (3.0 - 36.0 * cosio2 + 49.0 * cosio4);
        const double xhdot1 = -temp1 * cosio;
        elementSet.nodedot = xhdot1 + (0.5 * temp2 * (4.0 - 19.0 * cosio2) + 2.0 * temp3 * (3.0 - 7.0 * cosio2)) * cosio;

        elementSet.omgcof = elements.bstar * cc3 * std::cos(elements.argpo);
        elementSet.xmcof = 0.0;
        if (elements.ecco > 1.0e-4)
        {
            elementSet.xmcof = -kTwoThirds * coef * elements.bstar / eeta;
        }
        elementSet.nodecf = 3.5 * omeosq * xhdot1 * elementSet.cc1;
        elementSet.t2cof = 1.5 * elementSet.cc1;

        // An exactly retrograde orbit sends (1 + cos i) to zero. Substituting the threshold itself
        // as the divisor keeps the result finite; it is arbitrary, and it is the reference's.
        if (std::fabs(cosio + 1.0) > kRetrogradeSingularityGuard)
        {
            elementSet.xlcof = -0.25 * kSGP4J3OverJ2 * sinio * (3.0 + 5.0 * cosio) / (1.0 + cosio);
        }
        else
        {
            elementSet.xlcof = -0.25 * kSGP4J3OverJ2 * sinio * (3.0 + 5.0 * cosio) / kRetrogradeSingularityGuard;
        }

        elementSet.aycof = -0.5 * kSGP4J3OverJ2 * sinio;
        const double delmotemp = 1.0 + elementSet.eta * std::cos(elements.mo);
        elementSet.delmo = delmotemp * delmotemp * delmotemp;
        elementSet.sinmao = std::sin(elements.mo);
        elementSet.x7thm1 = 7.0 * cosio2 - 1.0;

        // The higher order drag terms, which a fast-decaying orbit does without.
        if (!elementSet.simplifiedDrag)
        {
            const double cc1sq = elementSet.cc1 * elementSet.cc1;
            elementSet.d2 = 4.0 * ao * tsi * cc1sq;
            const double temp = elementSet.d2 * tsi * elementSet.cc1 / 3.0;
            elementSet.d3 = (17.0 * ao + sfour) * temp;
            elementSet.d4 = 0.5 * temp * ao * tsi * (221.0 * ao + 31.0 * sfour) * elementSet.cc1;
            elementSet.t3cof = elementSet.d2 + 2.0 * cc1sq;
            elementSet.t4cof = 0.25 * (3.0 * elementSet.d3 + elementSet.cc1 * (12.0 * elementSet.d2 + 10.0 * cc1sq));
            elementSet.t5cof = 0.2 * (3.0 * elementSet.d4 + 12.0 * elementSet.cc1 * elementSet.d3 + 6.0 * elementSet.d2 * elementSet.d2 + 15.0 * cc1sq * (2.0 * elementSet.d2 + cc1sq));
        }
    }

    return elementSet;
}

SGP4Position SGP4Step(const SGP4ElementSet& elementSet, double tsinceMinutes)
{
    SGP4Position result;

    // Refused rather than attempted. SGP4Initialise() zeroes everything for a deep-space element
    // set, so no_unkozai is zero here and the first thing this would do with it is divide.
    if (elementSet.method == SGP4Method::DeepSpace)
    {
        result.error = SGP4Error::DeepSpaceNotSupported;
        return result;
    }

    const double twopi = glm::two_pi<double>();

    // Earth radii per minute into km per second, which is the only place the velocity's units
    // come from.
    const double vkmpersec = kSGP4EarthRadius * kSGP4Xke / 60.0;

    const double t = tsinceMinutes;

    // --- Secular gravity and atmospheric drag ---

    const double xmdf = elementSet.mo + elementSet.mdot * t;
    const double argpdf = elementSet.argpo + elementSet.argpdot * t;
    const double nodedf = elementSet.nodeo + elementSet.nodedot * t;
    double argpm = argpdf;
    double mm = xmdf;
    const double t2 = t * t;
    double nodem = nodedf + elementSet.nodecf * t2;
    double tempa = 1.0 - elementSet.cc1 * t;
    double tempe = elementSet.bstar * elementSet.cc4 * t;
    double templ = elementSet.t2cof * t2;

    // The higher order drag terms, which the coefficients only exist for when the orbit was not
    // decaying fast enough to have them dropped at initialisation.
    if (!elementSet.simplifiedDrag)
    {
        const double delomg = elementSet.omgcof * t;
        const double delmtemp = 1.0 + elementSet.eta * std::cos(xmdf);
        const double delm = elementSet.xmcof * (delmtemp * delmtemp * delmtemp - elementSet.delmo);
        const double temp = delomg + delm;
        mm = xmdf + temp;
        argpm = argpdf - temp;
        const double t3 = t2 * t;
        const double t4 = t3 * t;
        tempa = tempa - elementSet.d2 * t2 - elementSet.d3 * t3 - elementSet.d4 * t4;
        tempe = tempe + elementSet.bstar * elementSet.cc5 * (std::sin(mm) - elementSet.sinmao);
        templ = templ + elementSet.t3cof * t3 + t4 * (elementSet.t4cof + t * elementSet.t5cof);
    }

    double nm = elementSet.no_unkozai;
    double em = elementSet.ecco;
    const double inclm = elementSet.inclo;

    if (nm <= 0.0)
    {
        result.error = SGP4Error::MeanMotionNotPositive;
        return result;
    }

    const double am = std::pow((kSGP4Xke / nm), kTwoThirds) * tempa * tempa;
    nm = kSGP4Xke / std::pow(am, 1.5);
    em = em - tempe;

    // Drag has taken the eccentricity somewhere an orbit cannot be. The small negative bound is
    // the reference's tolerance rather than a physical statement.
    if ((em >= 1.0) || (em < -0.001))
    {
        result.error = SGP4Error::MeanElementsOutOfRange;
        return result;
    }

    if (em < 1.0e-6)
    {
        em = 1.0e-6;
    }

    mm = mm + elementSet.no_unkozai * templ;
    double xlm = mm + argpm + nodem;

    nodem = std::fmod(nodem, twopi);
    argpm = std::fmod(argpm, twopi);
    xlm = std::fmod(xlm, twopi);
    mm = std::fmod(xlm - argpm - nodem, twopi);

    const double sinim = std::sin(inclm);
    const double cosim = std::cos(inclm);

    // Where the deep-space path would add the lunar-solar periodics, which is the only thing that
    // makes these copies rather than the values themselves.
    const double ep = em;
    const double xincp = inclm;
    const double argpp = argpm;
    const double nodep = nodem;
    const double mp = mm;
    const double sinip = sinim;
    const double cosip = cosim;

    // --- Long period periodics ---

    const double axnl = ep * std::cos(argpp);
    double temp = 1.0 / (am * (1.0 - ep * ep));
    const double aynl = ep * std::sin(argpp) + temp * elementSet.aycof;
    const double xl = mp + argpp + nodep + temp * elementSet.xlcof * axnl;

    // --- Kepler's equation ---
    //
    // Newton-Raphson, capped at ten passes and with each correction clamped, because this is
    // solved for thirty thousand objects a frame and an orbit that converges slowly must cost a
    // bounded amount rather than an unbounded one.
    const double u = std::fmod(xl - nodep, twopi);
    double eo1 = u;
    double tem5 = 9999.9;
    double sineo1 = 0.0;
    double coseo1 = 0.0;
    int ktr = 1;
    while ((std::fabs(tem5) >= 1.0e-12) && (ktr <= 10))
    {
        sineo1 = std::sin(eo1);
        coseo1 = std::cos(eo1);
        tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
        tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;
        if (std::fabs(tem5) >= 0.95)
        {
            tem5 = tem5 > 0.0 ? 0.95 : -0.95;
        }
        eo1 = eo1 + tem5;
        ktr = ktr + 1;
    }

    // --- Short period periodics ---

    const double ecose = axnl * coseo1 + aynl * sineo1;
    const double esine = axnl * sineo1 - aynl * coseo1;
    const double el2 = axnl * axnl + aynl * aynl;
    const double pl = am * (1.0 - el2);

    if (pl < 0.0)
    {
        result.error = SGP4Error::NegativeSemiLatusRectum;
        return result;
    }

    const double rl = am * (1.0 - ecose);
    const double rdotl = std::sqrt(am) * esine / rl;
    const double rvdotl = std::sqrt(pl) / rl;
    const double betal = std::sqrt(1.0 - el2);
    temp = esine / (1.0 + betal);
    const double sinu = am / rl * (sineo1 - aynl - axnl * temp);
    const double cosu = am / rl * (coseo1 - axnl + aynl * temp);
    double su = std::atan2(sinu, cosu);
    const double sin2u = (cosu + cosu) * sinu;
    const double cos2u = 1.0 - 2.0 * sinu * sinu;
    temp = 1.0 / pl;
    const double temp1 = 0.5 * kSGP4J2 * temp;
    const double temp2 = temp1 * temp;

    const double mrt = rl * (1.0 - 1.5 * temp2 * betal * elementSet.con41) + 0.5 * temp1 * elementSet.x1mth2 * cos2u;
    su = su - 0.25 * temp2 * elementSet.x7thm1 * sin2u;
    const double xnode = nodep + 1.5 * temp2 * cosip * sin2u;
    const double xinc = xincp + 1.5 * temp2 * cosip * sinip * cos2u;
    const double mvt = rdotl - nm * temp1 * elementSet.x1mth2 * sin2u / kSGP4Xke;
    const double rvdot = rvdotl + nm * temp1 * (elementSet.x1mth2 * cos2u + 1.5 * elementSet.con41) / kSGP4Xke;

    // --- Orientation vectors ---

    const double sinsu = std::sin(su);
    const double cossu = std::cos(su);
    const double snod = std::sin(xnode);
    const double cnod = std::cos(xnode);
    const double sini = std::sin(xinc);
    const double cosi = std::cos(xinc);
    const double xmx = -snod * cosi;
    const double xmy = cnod * cosi;
    const double ux = xmx * sinsu + cnod * cossu;
    const double uy = xmy * sinsu + snod * cossu;
    const double uz = sini * sinsu;
    const double vx = xmx * cossu - cnod * sinsu;
    const double vy = xmy * cossu - snod * sinsu;
    const double vz = sini * cossu;

    result.position = glm::dvec3(mrt * ux, mrt * uy, mrt * uz) * kSGP4EarthRadius;
    result.velocity = glm::dvec3(mvt * ux + rvdot * vx, mvt * uy + rvdot * vy, mvt * uz + rvdot * vz) * vkmpersec;

    // Below one earth radius, so the orbit has come down. Reported after the position rather than
    // instead of it, because the position is how it was noticed.
    if (mrt < 1.0)
    {
        result.error = SGP4Error::Decayed;
    }

    return result;
}

SGP4Elements MakeSGP4Elements(const OrbitalElementsComponent& orbitalElements)
{
    // 1950 January 0.0 - JD 2433281.5, and the day count SGP4 initialises from - is 7306 days
    // before the Unix epoch. Not the 7305 the reference subtracts on its way to the sidereal time
    // at epoch: that one counts back to 1970 January 0.0, which is the last day of 1969 rather
    // than the first of 1970. The two dates are a day apart and so are the two constants.
    constexpr double kDaysFrom1950ToUnixEpoch = 7306.0;
    constexpr double kSecondsPerDay = 86400.0;

    const double degreesToRadians = glm::pi<double>() / 180.0;

    SGP4Elements elements;
    elements.bstar = orbitalElements.GetBStar();
    elements.ecco = orbitalElements.GetEccentricity();
    elements.argpo = orbitalElements.GetArgumentOfPericenter() * degreesToRadians;
    elements.inclo = orbitalElements.GetInclination() * degreesToRadians;
    elements.mo = orbitalElements.GetMeanAnomaly() * degreesToRadians;
    elements.nodeo = orbitalElements.GetRightAscensionOfAscendingNode() * degreesToRadians;
    elements.noKozai = orbitalElements.GetMeanMotion() / kRevsPerDayToRadsPerMinute;

    const double secondsSinceUnixEpoch = std::chrono::duration<double>(orbitalElements.GetEpoch().time_since_epoch()).count();
    elements.epochDaysSince1950 = secondsSinceUnixEpoch / kSecondsPerDay + kDaysFrom1950ToUnixEpoch;

    return elements;
}

} // namespace WingsOfSteel
