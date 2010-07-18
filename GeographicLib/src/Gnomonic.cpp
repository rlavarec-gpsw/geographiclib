/**
 * \file Gnomonic.cpp
 * \brief Implementation for GeographicLib::Gnomonic class
 *
 * Copyright (c) Charles Karney (2009, 2010) <charles@karney.com>
 * and licensed under the LGPL.  For more information, see
 * http://geographiclib.sourceforge.net/
 **********************************************************************/

#include "GeographicLib/Gnomonic.hpp"

#define GEOGRAPHICLIB_GNOMONIC_CPP "$Id$"

RCSID_DECL(GEOGRAPHICLIB_GNOMONIC_CPP)
RCSID_DECL(GEOGRAPHICLIB_GNOMONIC_HPP)

namespace GeographicLib {

  using namespace std;

  const Math::real Gnomonic::eps0 = numeric_limits<real>::epsilon();
  const Math::real Gnomonic::eps = real(0.01) * sqrt(eps0);

  void Gnomonic::Forward(real lat0, real lon0, real lat, real lon,
                         real& x, real& y, real& azi, real& rk)
    const throw() {
    real sig, s, azi0, m;
    sig = _earth.Inverse(lat0, lon0, lat, lon, s, azi0, azi, m);
    const GeodesicLine line(_earth.Line(lat0, lon0, azi0));
    real M, Mx;
    line.Scale(sig, M, Mx);
    rk = M;
    if (M <= 0)
      x = y = Math::NaN();
    else {
      real rho = m/M;
      azi0 *= Constants::degree();
      x = rho * sin(azi0);
      y = rho * cos(azi0);
    }
  }

  void Gnomonic::Reverse(real lat0, real lon0, real x, real y,
                         real& lat, real& lon, real& azi, real& rk)
    const throw() {
    real
      azi0 = atan2(x, y) / Constants::degree(),
      rho = min(Math::hypot(x, y), _a/(2 * eps0));
    GeodesicLine line(_earth.Line(lat0, lon0, azi0));
    real lat1, lon1, azi1, M, s;
    int count = numit;
    if (rho * _f < _a / 2)
      s = _a * atan(rho/_a);
    else {
      real m, Mx, ang = 90;
      int trip = _f == 0 ? 1 : max(1, int(-log(rho/_a) / log(_f) + real(0.5)));
      while (count--) {
        s = line.Position(ang, lat1, lon1, azi1, m, true);
        line.Scale(ang, M, Mx);
        if (trip < 0 && M > 0)
          break;
        // Estimate new arc length assuming dM/da = -1.
        ang += (M - m/rho)/Constants::degree();
        if (M > 0)
          --trip;
      }
      s -= (m/M - rho) * M * M;
    }
    int trip = 0;
    // Reset count if previous iteration was OK; otherwise skip next iteration
    count = count < 0 ? 0 : numit;
    while (count--) {
      real m, Mx;
      real ang = line.Position(s, lat1, lon1, azi1, m);
      line.Scale(ang, M, Mx);
      if (trip)
        break;
      else if (M <= 0) {
        count = -1;
        break;
      }
      real ds = (m/M - rho) * M * M;
      s -= ds;
      if (abs(ds) < eps * _a)
        ++trip;
    }
    if (count >= 0) {
      lat = lat1; lon = lon1; azi = azi1; rk = M;
    } else
      lat = lon = azi = rk = Math::NaN();
    return;
  }

#if GNOMONICR
  Math::real Gnomonic::ConformalLat(real geoglat, real lat0) const throw() {
    real
      phi = geoglat * Constants::degree(),
      phi0 = lat0 * Constants::degree(),
      tau = tan(phi),
      sig = sinh(_e * ( Math::atanh(_e * sin(phi)) -
                        Math::atanh(_e * sin(phi0)) )),
      taup = Math::hypot(real(1), sig) * tau - sig * Math::hypot(real(1), tau),
      phip = atan(taup);
    return phip / Constants::degree();
  }

  Math::real Gnomonic::GeographicLat(real conflat, real lat0) const throw() {
    real
      phip = conflat * Constants::degree(),
      taup = tan(phip),
      tau = taup,
      de = Math::atanh(_e * sin(lat0 * Constants::degree()));
    for (int i = 0; i < 5; ++i) {
      real
        tau1 = Math::hypot(real(1), tau),
        sig = sinh( _e * (Math::atanh(_e * tau / tau1) - de) ),
        sig1 =  Math::hypot(real(1), sig),
        dtau = - (sig1 * tau - sig * tau1 - taup) *
        (1 + (1 - _e * _e) * tau * tau) /
        ( (sig1 * tau1 - sig * tau) * (1 - _e * _e) * tau1 );
      tau += dtau;
      if (abs(dtau) < real(0.1)*sqrt(numeric_limits<real>::epsilon()) *
          max(real(1), tau))
        break;
    }
    real phi = atan(tau);
    return phi / Constants::degree();
  }


  void Gnomonic::ForwardR(real lat0, real lon0, real lat, real lon,
                         real& x, real& y, real& azi, real& rk)
    const throw() {
    real
      clat0 = lat0,
      clat = ConformalLat(lat, lat0),
      sphi0 = sin(lat0 * Constants::degree()),
      n = _a/sqrt(1 - _e2 * sphi0 * sphi0);
    real sig, s, azi0, m;
    sig = _sphere.Inverse(clat0, lon0, clat, lon, s, azi0, azi, m);
    sig *=  Constants::degree();
    rk = cos(sig);
    if (rk <= 0)
      x = y = Math::NaN();
    else {
      real rho = n * tan(sig);
      azi0 *= Constants::degree();
      x = rho * sin(azi0);
      y = rho * cos(azi0);
    }
  }

  void Gnomonic::ReverseR(real lat0, real lon0, real x, real y,
                         real& lat, real& lon, real& azi, real& rk)
    const throw() {
    real
      azi0 = atan2(x, y) / Constants::degree(),
      rho = min(Math::hypot(x, y), _a/(2 * eps0)),
      sphi0 = sin(lat0 * Constants::degree()),
      n = _a/sqrt(1 - _e2 * sphi0 * sphi0),
      sig = atan(rho/n) / Constants::degree(),
      clat0 = lat0,
      clat, m12;
    _sphere.Direct(clat0, lon0, azi0, sig, clat, lon, azi, m12, true);
    rk = cos(sig * Constants::degree());
    lat = GeographicLat(clat, lat0);
  }
#endif
} // namespace GeographicLib