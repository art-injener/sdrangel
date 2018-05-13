///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// See: http://liquidsdr.org/blog/pll-howto/                                     //
// Fixes filter registers saturation                                             //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <complex.h>
#include <math.h>
#include "phaselockcomplex.h"

PhaseLockComplex::PhaseLockComplex() :
    m_a1(1.0),
    m_a2(1.0),
    m_b0(1.0),
    m_b1(1.0),
    m_b2(1.0),
    m_v0(0.0),
    m_v1(0.0),
    m_v2(0.0),
    m_deltaPhi(0.0),
    m_phiHatLast(0.0),
    m_phiHat(0.0),
    m_y(1.0, 0.0),
    m_yRe(1.0),
    m_yIm(0.0),
    m_freq(0.0)
{
}

void PhaseLockComplex::computeCoefficients(Real wn, Real zeta, Real K)
{
    double t1 = K/(wn*wn);          //
    double t2 = 2*zeta/wn - 1/K;   //

    double b0 = 2*K*(1.+t2/2.0f);
    double b1 = 2*K*2.;
    double b2 = 2*K*(1.-t2/2.0f);

    double a0 =  1 + t1/2.0f;
    double a1 = -t1;
    double a2 = -1 + t1/2.0f;

    qDebug("PhaseLockComplex::computeCoefficients: b_raw: %f %f %f", b0, b1, b2);
    qDebug("PhaseLockComplex::computeCoefficients: a_raw: %f %f %f", a0, a1, a2);

    m_b0 = b0 / a0;
    m_b1 = b1 / a0;
    m_b2 = b2 / a0;

    //    a0 =  1.0  is implied
    m_a1 = a1 / a0;
    m_a2 = a2 / a0;

    qDebug("PhaseLockComplex::computeCoefficients: b: %f %f %f", m_b0, m_b1, m_b2);
    qDebug("PhaseLockComplex::computeCoefficients: a: 1.0 %f %f", m_a1, m_a2);

    reset();
}

void PhaseLockComplex::reset()
{
    // reset filter accumulators and phase
    m_v0 = 0.0f;
    m_v1 = 0.0f;
    m_v2 = 0.0f;
    m_deltaPhi = 0.0f;
    m_phiHatLast = 0.0f;
    m_phiHat = 0.0f;
    m_y.real(1.0);
    m_y.real(0.0);
    m_yRe = 1.0f;
    m_yIm = 0.0f;
    m_freq = 0.0f;
}

void PhaseLockComplex::feed(float re, float im)
{
    m_yRe = cos(m_phiHat);
    m_yIm = sin(m_phiHat);
    m_y.real(m_yRe);
    m_y.imag(m_yIm);
    std::complex<float> x(re, im);
    m_deltaPhi = std::arg(x * std::conj(m_y));

    // advance buffer
    m_v2 = m_v1;  // shift center register to upper register
    m_v1 = m_v0;  // shift lower register to center register

    // compute new lower register
    m_v0 = m_deltaPhi - m_v1*m_a1 - m_v2*m_a2;

    // compute new output
    m_phiHat = m_v0*m_b0 + m_v1*m_b1 + m_v2*m_b2;

    // prevent saturation
    if (m_phiHat > 2.0*M_PI)
    {
        m_v0 *= (m_phiHat - 2.0*M_PI) / m_phiHat;
        m_v1 *= (m_phiHat - 2.0*M_PI) / m_phiHat;
        m_v2 *= (m_phiHat - 2.0*M_PI) / m_phiHat;
        m_phiHat -= 2.0*M_PI;
    }

    if (m_phiHat < -2.0*M_PI)
    {
        m_v0 *= (m_phiHat + 2.0*M_PI) / m_phiHat;
        m_v1 *= (m_phiHat + 2.0*M_PI) / m_phiHat;
        m_v2 *= (m_phiHat + 2.0*M_PI) / m_phiHat;
        m_phiHat += 2.0*M_PI;
    }

    m_freq = (m_phiHat - m_phiHatLast) / (2.0*M_PI);

    if (m_freq < -1.0f) {
        m_freq += 2.0f;
    } else if (m_freq > 1.0f) {
        m_freq -= 2.0f;
    }

    m_phiHatLast = m_phiHat;
}


