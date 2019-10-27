/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, development version     *
*                (c) 2006-2019 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef SOFA_COMPONENT_FORCEFIELD_ISOCHORICFORCEFIELD_INL
#define SOFA_COMPONENT_FORCEFIELD_ISOCHORICFORCEFIELD_INL

#include "IABPlugin/ForceFields/include/integrand.inl"  // will help with our integrations
#include "IABPlugin/ForceFields/include/IsochoricForceField.h"
#include <cassert>
#include <iostream>
#include <sofa/helper/rmath.h>
#include <sofa/core/visual/VisualParams.h>
#include <sofa/defaulttype/RGBAColor.h>
#include <sofa/helper/system/config.h>
#include <sofa/helper/logging/Messaging.h>

// see SoftRobots/model/SurfacePressureModel
namespace sofa
{

namespace component
{

namespace forcefield
{
// Constructor of the class IsochoricForceField
// initializing data with their default value (here d_inputForTheUser=20)
template<typename DataTypes>
IsochoricForceField<DataTypes>::IsochoricForceField()
    : indices(initData(&indices, "indices", "index of nodes controlled by the isochoric deformations")),
    d_Ri(initData(&d_Ri, Real(10), "Ri", "internal radius in the reference configuration")),
    d_Ro(initData(&d_Ro, Real(15), "Ro", "external radius in the reference configuration")),
    d_ri(initData(&d_ri, Real(13), "ri", "internal radius in the current configuration")),
    // d_ro(initData(&d_ro, Real(25), "ro", "external radius in the current configuration")),
    d_C1(initData(&d_C1, Real(1.1e4), "C1", "material elasticity of the internal IAB wall")),
    d_C2(initData(&d_C2, Real(2.2e4), "C2", "material elasticity of the outer IAB wall")),
    d_mode(initData(&d_mode, "expand", "mode", "mode of deformation: <expansion> or <compression>")),
    counter(0)
{
  //default Constructor
  init();
}


template<typename DataTypes>
IsochoricForceField<DataTypes>::~IsochoricForceField()
{
}


template<typename DataTypes>
void IsochoricForceField<DataTypes>::init()
{
    // ripped off angularSpringForceField
    core::behavior::ForceField<DataTypes>::init();

    if((d_ri.getValue()==0) && (d_ro.getValue()==0))
    {
      std::cerr << "Understand that these ri and ro values cannot be both zero" << std::endl;
      // std::terminate();
    }

    abstol = 1e-2F;
    reltol = 1e-5F;

    m_Ri = d_Ri.getValue();
    m_Ro = d_Ro.getValue();
    m_ri = d_ri.getValue();
    // m_ro = d_ro.getValue();
    m_C1 = d_C1.getValue();
    m_C2 = d_C2.getValue();

    // radii to take IAB into in the current configuration
    m_ro = std::cbrt(std::pow(m_Ro, 3) - std::pow(m_Ri, 3) + \
            std::pow(m_ri, 3));
    d_ro.setValue(m_ro);
    //  mState = dynamic_cast<core::behavior::MechanicalState<DataTypes> *> (this->getContext()->getMechanicalState());
    // if (!mState) {
		// msg_error("IsochoricForceField") << "MechanicalStateFilter has no binding MechanicalState" << "\n";
    // }
    // matS.resize(mState->getMatrixSize(), mState->getMatrixSize());
}


template<typename DataTypes>
void IsochoricForceField<DataTypes>::reinit()
{
  // not yet implemented
}

template<typename DataTypes>
void IsochoricForceField<DataTypes>::addForce(const core::MechanicalParams* /*params*/,
                                             DataVecDeriv& f,
                                             const DataVecCoord& x,
                                             const DataVecDeriv& v)
{
    if(!mState) {
    msg_info("IsochoricForceField") << "No Mechanical State found, no force will be computed..." << "\n";
        return;
    }
    // Compute the forces f from the current DOFs p; here i am using the derived stress from eq 25v in paper 1
    helper::WriteAccessor< DataVecDeriv > f1 = f;
    helper::ReadAccessor< DataVecCoord >  p1 = x;
    helper::ReadAccessor< DataVecDeriv >  v1 = v;

    f1.resize(p1.size());

    // // these from PREquivalentStiffnessForceField.inl
    //     if(m_componentstate != ComponentState::Valid)
    //         return ;

    SOFA_UNUSED(f);
    SOFA_UNUSED(x);
    SOFA_UNUSED(v);

    // calculate the stress and pressure needed to go from a reference configuartion to a current configuration
    auto stressFunc = radial_stress_r2c<double>(m_Ri, m_Ro, m_ri, m_C1, m_C2);
    // auto stress_rr = radial_stress_r2c<double>(m_Ri, m_Ro, m_ri, m_C1, m_C2);
    // const float stress_rr = integrator<float, \
    //                             radial_stress_r2c<float>>(m_ri, m_ro, \
    //                                   abstol, reltol, stressFunc);
    // using stress_rr = radial_stress_r2c<double>;
    double stress_rr;
  	// stepper.do_step([](const state& x, state & dxdt, const double t)->void{
  	// 	dxdt = x;
  	// }, stressFunc, counter, stress_rr, 0.01);
    // ++counter;
  	stepper.do_step_impl([](const state& x, state & dxdt, double& stateOut, const double t)->void{
  		dxdt = x;
  	}, stressFunc, counter, stress_rr, 0.01);
    ++counter;
    msg_info("IsochoricForceField") << "Calculated normal stress based on given parameters:" <<
              "\n\tC1: " << m_C1 << " C2: " << m_C2 << " Ri: " <<  m_Ri  << " Ro: " <<  m_Ro <<
              "\n\tri: " <<  m_ri  <<  " ro: " << m_ro << " stress_rr: " << stress_rr <<
              "\n\tP: " << -1*stress_rr << "\n";
}

// for when the bladder is being radially inflated
template<typename DataTypes>
void IsochoricForceField<DataTypes>::addPressure(const sofa::core::MechanicalParams* mparams, DataVecDeriv& P, DataVecDeriv& Patm, \
                  const DataVecCoord& x1, const DataVecDeriv& v1)
{
  auto PressureFunc = pressure_r2c<float>(m_Ri, m_Ro, m_ri, m_C1, m_C2);
  // integrate from ri to ro in r
  const float int_pressure = integrator<float, \
                              pressure_r2c<float>>(m_ri, m_ro, \
                                    abstol, reltol, PressureFunc);
}

template<typename DataTypes>
void IsochoricForceField<DataTypes>::addDForce(const core::MechanicalParams* mparams,
                                              DataVecDeriv& d_df , const DataVecDeriv& d_dx)
{
    // Compute the force derivative d_df from the current, which will be multiplied with the field d_dx
}


template<typename DataTypes>
void IsochoricForceField<DataTypes>::addKToMatrix(sofa::defaulttype::BaseMatrix * /* mat */,
                                                 SReal /* k */, unsigned int & /* offset */)
{
    // Compute the force derivative d_df from the current and store the resulting matrix
}

template<typename DataTypes>
void IsochoricForceField<DataTypes>::draw(const core::visual::VisualParams* vparams)
{
  // nothing to do here
};

template<typename DataTypes>
void IsochoricForceField<DataTypes>::addKToMatrix(const sofa::core::behavior::MultiMatrixAccessor* /*matrix*/,
                                                 SReal /*kFact*/)
{
    // Same as previously
    // but using accessor
}


template <typename DataTypes>
SReal IsochoricForceField<DataTypes>::getPotentialEnergy(const core::MechanicalParams* mparams,
                                                        const DataVecCoord& x) const
{
  SOFA_UNUSED(mparams);
  SOFA_UNUSED(x);

  return 0.0; // dummy retun for now
}




} // namespace forcefield

} // namespace component

} // namespace sofa

#endif // SOFA_COMPONENT_FORCEFIELD_ISOCHORICFORCEFIELD_INL
