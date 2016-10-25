//
//  body.c
//  trajectory
//
//  Created by Thomas Malthouse on 6/28/16.
//  Copyright © 2016 Thomas Malthouse. All rights reserved.
//

#include "body.h"
#include <tgmath.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "vector3d.h"
#include "types.h"
#include "debug.h"

//Returns the g-force between the 2 given bodies.
Vector3d body_gforce(Body a, Body b)
{
    double abs_dist = v3d_absdist(a.pos, b.pos);
    Vector3d unit_vect = v3d_unit_vector(v3d_vdiff(a.pos, b.pos));
    
    return v3d_fmult(unit_vect, (BIG_G * a.mass * b.mass)/(abs_dist*abs_dist));
}

Vector3d net_gforce(Body *sys, uint64_t count, uint64_t focusbody)
{
    Vector3d result = V3D_0_VECTOR;
    for (uint32_t i=0; i<count; i++) {
        if (i!=focusbody) {
            result = v3d_vsum(result, body_gforce(sys[i], sys[focusbody]));
        }
    }
    return result;
}

double calculate_mu(Body b)
{
    return BIG_G*b.mass;
}

double parent_mu(Orbit o)
{
    if (o.parent == NULL) {
        return 0.0;
    }
    return calculate_mu(*o.parent);
}


Vector3d ang_mom_h(Vector3d r, Vector3d v)
{
    return v3d_xprod(r, v);
}

Vector3d node_vector(Vector3d h)
{
    return v3d_xprod(V3D_K_VECTOR, h);
}

Vector3d eccentricity_vector(double parent_mu, Vector3d r, Vector3d v, Vector3d h)
{
    double inv_mu = 1/parent_mu;
    double abs_pos = v3d_abs(r);
    
    Vector3d vel_cross_mom = v3d_xprod(v, h);
    Vector3d vcm_div_mu = v3d_fmult(vel_cross_mom, inv_mu);
    
    Vector3d pos_div_abs = v3d_fmult(r, 1/abs_pos);
    
    return v3d_vdiff(vcm_div_mu, pos_div_abs);
}

double orb_parameter(Vector3d h, double parent_mu)
{
    double abs_h = v3d_abs(h);

    return (abs_h*abs_h)/parent_mu;
}

double orb_eccentricity(Vector3d e_vector)
{
    return v3d_abs(e_vector);
}

double orb_inclination(Vector3d h_vector)
{
    return acos(h_vector.z / v3d_abs(h_vector));
}

double orb_long_asc_node(Vector3d node_vector)
{
    double angle = acos(node_vector.x / v3d_abs(node_vector));

    if (node_vector.y<0) {
        angle *= -1;
    }

    return angle;
}

double arg_of_periapsis(Vector3d node_vector, Vector3d ecc_vector)
{
    double angle = acos(v3d_dotprod(node_vector, ecc_vector) / (v3d_abs(node_vector)*v3d_abs(ecc_vector)));

    if (ecc_vector.z<0) {
        angle *= -1;
    }

    return angle;
}

double true_anomaly(Vector3d ecc_vector, Vector3d pos, Vector3d vel)
{
    double angle = acos(v3d_dotprod(ecc_vector, pos)/(v3d_abs(ecc_vector)*v3d_abs(pos)));

    if(v3d_dotprod(pos, vel)<0) {
        angle *= -1;
    }

    return angle;
}

double ecc_anomaly(double ecc, double true_ano)
{
    double num = sqrt(1-ecc*ecc)*sin(true_ano);
    double den = 1 + ecc*cos(true_ano);

    return atan2(num, den);
}

double mean_anomaly(double ecc_ano, double ecc)
{
    return ecc_ano - ecc * sin(ecc_ano);
}


/* Helper functions for ecc_ano_from_mean_ano*/
    static double ecc = 0;
    static double mean_ano = 0;

    static double ecc_ano_func(double ecc_ano)
    {
        return (ecc_ano - ecc*sin(ecc_ano) - mean_ano);
    }

    static double ecc_ano_deriv_func(double ecc_ano)
    {
        return (1-ecc*cos(ecc_ano));
    }

double ecc_ano_from_mean_ano(double mean_ano_param, double ecc_param)
{
    ecc = ecc_param;
    mean_ano = mean_ano_param;
    return newton_raphson_iterate(ecc_ano_func, ecc_ano_deriv_func, mean_ano, 16);
}
/*Now back to your regularly scheduled programming...*/

double mean_ano_at_t(Orbit o, Time t)
{
    double raw_ano = o.mna + (t-o.epoch) + sqrt(o.parent->mu/pow(o.sma, 3));
    return fmod(raw_ano, PI*2);
}

double true_ano_from_ecc_ano(double ecc_ano, double eccentricity)
{
    double num = sqrt(1+eccentricity)*sin(ecc_ano/2);
    double den = sqrt(1-eccentricity)*cos(ecc_ano/2);
    return 2*atan2(num,den);
}

double orbital_height_from_ecc_ano(Orbit o, double ecc_ano)
{
    return o.sma*(1-o.ecc*cos(ecc_ano));
}

Vector3d bodycentric_position(double orb_hgt, double true_ano)
{
    Vector3d pos;
    pos.x = orb_hgt*cos(true_ano);
    pos.y = orb_hgt*sin(true_ano);
    pos.z = 0;
    return pos;
}

Vector3d bodycentric_velocity(Orbit o, double orb_hgt, double ecc_ano)
{
    Vector3d vel;

    double factor = sqrt(o.parent->mu * o.sma)/orb_hgt;
    vel.x = factor * -1 * sin(ecc_ano);
    vel.y = factor * sqrt(1-o.ecc*o.ecc) * cos(ecc_ano);
    vel.z = 0;
    return vel;
}

Vector3d bodycentric_to_cartesian(Orbit o, Vector3d vect)
{
    //This uses some kind of rotation matrix I found on the internet!
    Vector3d prod;
    
    prod.x = vect.x * (cos(o.ape)*cos(o.lan) - sin(o.ape)*cos(o.inc)*sin(o.lan)) -
             vect.y * (sin(o.ape)*cos(o.lan) + cos(o.ape)*cos(o.inc)*sin(o.lan));

    prod.y = vect.x * (cos(o.ape)*sin(o.lan) + sin(o.ape)*cos(o.inc)*cos(o.lan)) +
             vect.y * (cos(o.ape)*cos(o.inc)*cos(o.lan) - sin(o.ape)*sin(o.lan));

    prod.z = vect.x * (sin(o.ape)*sin(o.inc)) +
             vect.y * (cos(o.ape)*sin(o.inc));

    return prod;
}

void calculate_state_vectors(Body *b, Time t)
{
    //If the body is root, pos and vel are 0.
    if (b->orbit.parent == NULL) {
        b->pos = V3D_0_VECTOR;
        b->vel = V3D_0_VECTOR;
        return;
    }
    
    double ecc_anomaly = ecc_ano_from_mean_ano(mean_ano_at_t(b->orbit, t), b->orbit.ecc);
    double true_anomaly = true_ano_from_ecc_ano(ecc_anomaly, b->orbit.ecc);
    double altitude = orbital_height_from_ecc_ano(b->orbit, ecc_anomaly);

    Vector3d b_pos = bodycentric_position(altitude, true_anomaly);
    Vector3d b_vel = bodycentric_velocity(b->orbit, altitude, ecc_anomaly);

    //Add the calculated position relative to the parent to the parent's absolute position.
    b->pos = v3d_vsum(bodycentric_to_cartesian(b->orbit, b_pos), b->orbit.parent->pos);
    //And the same with the velocity.
    b->vel = v3d_vsum(bodycentric_to_cartesian(b->orbit, b_vel), b->orbit.parent->vel);

    return;
}

double orb_sma (Vector3d r, Vector3d v, double parent_mu)
{
    return 1/((2/v3d_abs(r)) - (pow(v3d_abs(v), 2)/parent_mu));
}

void calculate_orbit_params(Body *b)
{
    Vector3d pos, vel;
    
    if (b->orbit.parent == NULL) {
        //If it's the root body, there are no real orbital params to speak of.
        b->orbit = (Orbit){0};
        return;
    } else {
        pos = v3d_vdiff(b->pos, b->orbit.parent->pos);
        vel = v3d_vdiff(b->vel, b->orbit.parent->vel);
    }
    
    Vector3d h = ang_mom_h(pos, vel);
    Vector3d e_vect = eccentricity_vector(b->orbit.parent->mu, pos, vel, h);
    
    Vector3d n_vect = node_vector(h);
    double t_ano = true_anomaly(e_vect, pos, vel);
    
    double inc = orb_inclination(h);
    double eccentr = orb_eccentricity(e_vect);
    double e_ano = ecc_anomaly(eccentr, t_ano);
    double lan = orb_long_asc_node(n_vect);
    double ape = arg_of_periapsis(n_vect, e_vect);
    double m_ano = mean_anomaly(e_ano, eccentr);
    double sma = orb_sma(pos, vel, b->orbit.parent->mu);
    
    b->orbit = (Orbit){.sma = sma, .inc = inc, .ecc = eccentr,
                       .lan = lan, .ape = ape, .mna = m_ano,
                       .epoch = 0};
}


double newton_raphson_iterate(unaryfunc f, unaryfunc fderiv, double guess, uint8_t iterations)
{
    if (iterations == 0) {
        return guess;
    }
    
    double newguess = guess - (((*f)(guess))/((*fderiv)(guess)));
    return newton_raphson_iterate(f, fderiv, newguess, iterations-1);
}

void euler_step(Body *b, Vector3d acc, Vector3d vel, Time dt)
{
    b->pos = v3d_vsum(b->pos, v3d_fmult(vel, dt));
    b->vel = v3d_vsum(b->vel, v3d_fmult(acc, dt));
}


static _Thread_local Body *update_state_vectors_lsys = NULL;
void update_state_vectors(Body *sys, uint64_t count, uint64_t bodyid, Time dt)
{
    //We need a storage buffer for the bodies, but need a distinct one per thread (this func isn't recursive, so that won't be an issue.) However, this also means we nead C11+ to compile.
    //The memory allocated for the buffer here is freed by the destructor function, which should be attached to ALL threads by pthread_key_create.
    
    if (update_state_vectors_lsys == NULL) {
        logger("Allocating update_state_vectors buffer. This should only happen on start.");
        update_state_vectors_lsys = calloc (1, count*sizeof(Body));
    }
    
    Body *lsys = update_state_vectors_lsys;
    
    memcpy(lsys, sys, count*sizeof(Body)); //We want to use a copy so we aren't disturbing any other thread/core with our orbital shiftiness
    Body *focus = &lsys[bodyid];
    
    Vector3d init_pos = focus->pos;
    Vector3d init_vel = focus->vel;
    
    // Step one of our two simultaneous runge-kuttas.
    Vector3d acc_1 = v3d_fmult(net_gforce(lsys, count, bodyid), 1/(focus->mass)); // Aka dv/dt
    Vector3d vel_1 = init_vel; // Aka dr/dt
    
    euler_step(focus, acc_1, vel_1, dt/2);
    
    // Now step two: the second slopes
    Vector3d acc_2 = v3d_fmult(net_gforce(lsys, count, bodyid), 1/(focus->mass));
    Vector3d vel_2 = focus->vel;
    
    //Reset it for the mini-euler
    focus->pos = init_pos;
    focus->vel = init_vel;
    
    euler_step(focus, acc_2, vel_2, dt/2);
    
    // You guessed it...Step 3!
    Vector3d acc_3 = v3d_fmult(net_gforce(lsys, count, bodyid), 1/(focus->mass));
    Vector3d vel_3 = focus->vel;
    
    //Reset again!
    focus->pos = init_pos;
    focus->vel = init_vel;
    
    euler_step(focus, acc_3, vel_3, dt);
    
    //One last step...
    Vector3d acc_4 = v3d_fmult(net_gforce(lsys, count, bodyid), 1/(focus->mass));
    Vector3d vel_4 = focus->vel;
    
    Vector3d v_combined = v3d_vsum(init_vel,
                                   v3d_fmult(v3d_nsum(4, acc_1,
                                                        v3d_fmult(acc_2, 2),
                                                        v3d_fmult(acc_3, 2),
                                                        acc_4),
                                             dt/6));
    Vector3d p_combined = v3d_vsum(init_pos,
                                   v3d_fmult(v3d_nsum(4, vel_1,
                                                      v3d_fmult(vel_2, 2),
                                                      v3d_fmult(vel_3, 2),
                                                      vel_4),
                                             dt/6));
    Body *original = &sys[bodyid];
    
    original->vel = v_combined;
    original->pos = p_combined;
    return;
}


void system_update(Body *sys, uint64_t count, Time dt, Time *t)
{
    for (uint64_t i=0; i<count; i++) {
        update_state_vectors(sys, count, i, dt);
        dblogger("Absolute velocity of body %llu is %f\n", i, v3d_abs(sys[i].vel));
    }
    *t += dt;
}

uint64_t system_total_energy(Body *sys, uint64_t count)
{
    uint64_t total_e = 0;
    
    for (uint64_t i=0; i<count; i++) {
        total_e += 0.5 * sys[i].mass + pow(v3d_abs(sys[i].vel), 2);
    }
    
    return total_e;
}

/// Destructor for a thread that calls update_state_vectors anywhere. Note that this function is safe to call at any time--i.e. it doen't break internal state (the buffer will simply be reallocated) and will work even if the buffer has never been allocated (since free(NULL) is safe.)
void update_sv_thread_destructor(void *ptr)
{
    free(update_state_vectors_lsys);
    update_state_vectors_lsys = NULL;
}

void print_body_info(Body b) {
    logger("For body %s:\n Mass=%f. SMA=%f. Pos={%f,%f,%f}.\n Vel={%f,%f,%f}\n", b.name, b.mass, b.orbit.sma, b.pos.x, b.pos.y, b.pos.z, b.vel.x, b.vel.y, b.vel.z);
}
