/* Copyright (C) 2015-2017 Matthew Dawson
 * Licensed under the GNU General Public License version 2 or (at your
 * option) any later version. See the file COPYING for details.
 *
 * CMAQ_H2O2 reaction solver functions
 *
*/
/** \file
 * \brief CMAQ_H2O2 reaction solver functions
*/
#include "../rxn_solver.h"

// TODO Lookup environmental indicies during initialization
#define TEMPERATURE_K_ env_data[0]
#define PRESSURE_PA_ env_data[1]

#define NUM_REACT_ int_data[0]
#define NUM_PROD_ int_data[1]
#define INT_DATA_SIZE_ int_data[2]
#define FLOAT_DATA_SIZE_ int_data[3]
#define k1_A_ float_data[0]
#define k1_B_ float_data[1]
#define k1_C_ float_data[2]
#define k2_A_ float_data[3]
#define k2_B_ float_data[4]
#define k2_C_ float_data[5]
#define CONV_ float_data[6]
#define RATE_CONSTANT_ float_data[7]
#define NUM_INT_PROP_ 4
#define NUM_FLOAT_PROP_ 8
#define REACT_(x) (int_data[NUM_INT_PROP_ + x]-1)
#define PROD_(x) (int_data[NUM_INT_PROP_ + NUM_REACT_ + x]-1)
#define DERIV_ID_(x) int_data[NUM_INT_PROP_ + NUM_REACT_ + NUM_PROD_ + x]
#define JAC_ID_(x) int_data[NUM_INT_PROP_ + 2*(NUM_REACT_+NUM_PROD_) + x]
#define YIELD_(x) float_data[NUM_FLOAT_PROP_ + x]

/** \brief Flag Jacobian elements used by this reaction
 *
 * \param rxn_data A pointer to the reaction data
 * \param jac_struct 2D array of flags indicating potentially non-zero 
 *                   Jacobian elements
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_get_used_jac_elem(void *rxn_data, pmc_bool **jac_struct)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  for (int i_ind = 0; i_ind < NUM_REACT_; i_ind++) {
    for (int i_dep = 0; i_dep < NUM_REACT_; i_dep++) {
      jac_struct[REACT_(i_dep)][REACT_(i_ind)] = pmc_true;
    }
    for (int i_dep = 0; i_dep < NUM_PROD_; i_dep++) {
      jac_struct[PROD_(i_dep)][REACT_(i_ind)] = pmc_true;
    }
  }

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}  

/** \brief Update the time derivative and Jacbobian array indices
 *
 * \param model_data Pointer to the model data
 * \param deriv_ids Id of each state variable in the derivative array
 * \param jac_ids Id of each state variable combo in the Jacobian array
 * \param rxn_data Pointer to the reaction data
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_update_ids(ModelData model_data, int *deriv_ids,
          int **jac_ids, void *rxn_data)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  // Update the time derivative ids
  for (int i=0; i < NUM_REACT_; i++)
	  DERIV_ID_(i) = deriv_ids[REACT_(i)];
  for (int i=0; i < NUM_PROD_; i++)
	  DERIV_ID_(i + NUM_REACT_) = deriv_ids[PROD_(i)];

  // Update the Jacobian ids
  int i_jac = 0;
  for (int i_ind = 0; i_ind < NUM_REACT_; i_ind++) {
    for (int i_dep = 0; i_dep < NUM_REACT_; i_dep++) {
      JAC_ID_(i_jac++) = jac_ids[REACT_(i_dep)][REACT_(i_ind)];
    }
    for (int i_dep = 0; i_dep < NUM_PROD_; i_dep++) {
      JAC_ID_(i_jac++) = jac_ids[PROD_(i_dep)][REACT_(i_ind)];
    }
  }
  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}

/** \brief Update reaction data for new environmental conditions
 *
 * For CMAQ_H2O2 reaction this only involves recalculating the rate 
 * constant.
 *
 * \param env_data Pointer to the environmental state array
 * \param rxn_data Pointer to the reaction data
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_update_env_state(PMC_C_FLOAT *env_data, void *rxn_data)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  // Calculate the rate constant in (#/cc)
  // k = k1 + [M]*k2
  PMC_C_FLOAT conv = CONV_ * PRESSURE_PA_ / TEMPERATURE_K_;
  RATE_CONSTANT_ = (k1_A_ 
	  * (k1_C_==0.0 ? 1.0 : exp(k1_C_/TEMPERATURE_K_))
	  * (k1_B_==0.0 ? 1.0 :
                  pow(TEMPERATURE_K_/((PMC_C_FLOAT)300.0), k1_B_))
	  + k2_A_ // [M] is included in k2_A_
	  * (k2_C_==0.0 ? 1.0 : exp(k2_C_/TEMPERATURE_K_))
	  * (k2_B_==0.0 ? 1.0 : 
                  pow(TEMPERATURE_K_/((PMC_C_FLOAT)300.0), k2_B_))
	  * conv
	  ) * pow(conv, NUM_REACT_-1);

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}

/** \brief Do pre-derivative calculations
 *
 * Nothing to do for CMAQ_H2O2 reactions
 *
 * \param model_data Pointer to the model data, including the state array
 * \param rxn_data Pointer to the reaction data
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_pre_calc(ModelData model_data, void *rxn_data)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}

/** \brief Calculate contributions to the time derivative \f$f(t,y)\f$ from
 * this reaction.
 *
 * \param model_data Pointer to the model data
 * \param deriv Pointer to the time derivative to add contributions to
 * \param rxn_data Pointer to the reaction data
 * \param time_step Current time step being computed (s)
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
#ifdef PMC_USE_SUNDIALS
void * rxn_CMAQ_H2O2_calc_deriv_contrib(ModelData model_data,
          PMC_SOLVER_C_FLOAT *deriv, void *rxn_data, PMC_C_FLOAT time_step)
{
  PMC_C_FLOAT *state = model_data.state;
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  // Calculate the reaction rate
  PMC_C_FLOAT rate = RATE_CONSTANT_;
  for (int i_spec=0; i_spec<NUM_REACT_; i_spec++) rate *= state[REACT_(i_spec)];

  // Add contributions to the time derivative
  if (rate!=ZERO) {
    int i_dep_var = 0;
    for (int i_spec=0; i_spec<NUM_REACT_; i_spec++, i_dep_var++) {
      if (DERIV_ID_(i_dep_var) < 0) continue; 
      deriv[DERIV_ID_(i_dep_var)] -= (PMC_SOLVER_C_FLOAT) rate;
    }
    for (int i_spec=0; i_spec<NUM_PROD_; i_spec++, i_dep_var++) {
      if (DERIV_ID_(i_dep_var) < 0) continue; 
      deriv[DERIV_ID_(i_dep_var)] += (PMC_SOLVER_C_FLOAT)
              (rate*YIELD_(i_spec));
    }
  }

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);

}
#endif

/** \brief Calculate contributions to the Jacobian from this reaction
 *
 * \param model_data Pointer to the model data
 * \param J Pointer to the sparse Jacobian matrix to add contributions to
 * \param rxn_data Pointer to the reaction data
 * \param time_step Current time step being calculated (s)
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
#ifdef PMC_USE_SUNDIALS
void * rxn_CMAQ_H2O2_calc_jac_contrib(ModelData model_data,
          PMC_SOLVER_C_FLOAT *J, void *rxn_data, PMC_C_FLOAT time_step)
{
  PMC_C_FLOAT *state = model_data.state;
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  // Calculate the reaction rate
  PMC_C_FLOAT rate = RATE_CONSTANT_;
  for (int i_spec=0; i_spec<NUM_REACT_; i_spec++) rate *= state[REACT_(i_spec)];

  // Add contributions to the Jacobian
  if (rate!=ZERO) {
    int i_elem = 0;
    for (int i_ind=0; i_ind<NUM_REACT_; i_ind++) {
      for (int i_dep=0; i_dep<NUM_REACT_; i_dep++, i_elem++) {
	if (JAC_ID_(i_elem) < 0) continue;
	J[JAC_ID_(i_elem)] -= (PMC_SOLVER_C_FLOAT)
                (rate / state[REACT_(i_ind)]);
      }
      for (int i_dep=0; i_dep<NUM_PROD_; i_dep++, i_elem++) {
	if (JAC_ID_(i_elem) < 0) continue;
	J[JAC_ID_(i_elem)] += (PMC_SOLVER_C_FLOAT)
              (YIELD_(i_dep) * rate / state[REACT_(i_ind)]);
      }
    }
  }

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);

}
#endif

/** \brief Advance the reaction data pointer to the next reaction
 * 
 * \param rxn_data Pointer to the reaction data
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_skip(void *rxn_data)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}

/** \brief Print the CMAQ_H2O2 reaction parameters
 *
 * \param rxn_data Pointer to the reaction data
 * \return The rxn_data pointer advanced by the size of the reaction data
 */
void * rxn_CMAQ_H2O2_print(void *rxn_data)
{
  int *int_data = (int*) rxn_data;
  PMC_C_FLOAT *float_data = (PMC_C_FLOAT*) &(int_data[INT_DATA_SIZE_]);

  printf("\n\nCMAQ_H2O2 reaction\n");
  for (int i=0; i<INT_DATA_SIZE_; i++) 
    printf("  int param %d = %d\n", i, int_data[i]);
  for (int i=0; i<FLOAT_DATA_SIZE_; i++)
    printf("  float param %d = %le\n", i, float_data[i]);
 
  return (void*) &(float_data[FLOAT_DATA_SIZE_]);
}

#undef TEMPERATURE_K_
#undef PRESSURE_PA_

#undef NUM_REACT_
#undef NUM_PROD_
#undef k1_A_
#undef k1_B_
#undef k1_C_
#undef k2_A_
#undef k2_B_
#undef k2_C_
#undef CONV_
#undef RATE_CONSTANT_
#undef NUM_INT_PROP_
#undef NUM_FLOAT_PROP_
#undef REACT_
#undef PROD_
#undef DERIV_ID_
#undef JAC_ID_
#undef YIELD_
#undef INT_DATA_SIZE_
#undef FLOAT_DATA_SIZE_
