#ifndef GLUQLO_HILL_EVENTS_INTERNAL_H
#define GLUQLO_HILL_EVENTS_INTERNAL_H

#include "hill_events.h"
#include "gfx_primitives.h"
#include "scene_core.h"
#include "ships_battles.h"

double hill_random(const HillEvent *event, int a, int b);
void hill_walker(RenderTarget *surface, double x, double ground, double height, int direction,
                 double step, bool lantern, double fade, double t, int index);
double hill_ground(const HillEvent *event, double x);

void hill_village(const HillEvent *event);
void hill_caravan(const HillEvent *event);
void hill_turbines(const HillEvent *event);
void hill_lanterns(const HillEvent *event);
void hill_campfire(const HillEvent *event);
void hill_bloom(const HillEvent *event);
void hill_herd(const HillEvent *event);
void hill_volcano(const HillEvent *event);
void hill_beacons(const HillEvent *event);
void hill_fireflies(const HillEvent *event);
void hill_kites(const HillEvent *event);
void hill_fireworks(const HillEvent *event);
void hill_train(const HillEvent *event);
void hill_rocket(const HillEvent *event);
void hill_abduction(const HillEvent *event);
void hill_geysers(const HillEvent *event);
void hill_stones(const HillEvent *event);
void hill_ferris(const HillEvent *event);
void hill_observatory(const HillEvent *event);
void hill_windmills(const HillEvent *event);
void hill_lighttree(const HillEvent *event);
void hill_meteorite(const HillEvent *event);
void hill_dustdevils(const HillEvent *event);
void hill_lasers(const HillEvent *event);
void hill_tesla(const HillEvent *event);
void hill_balloons(const HillEvent *event);
void hill_giant(const HillEvent *event);
void hill_clocktower(const HillEvent *event);
void hill_drones(const HillEvent *event);
void hill_bats(const HillEvent *event);
void hill_gate(const HillEvent *event);
void hill_lighthouse(const HillEvent *event);
void hill_cherry(const HillEvent *event);
void hill_wolf(const HillEvent *event);
void hill_pumpkins(const HillEvent *event);
void hill_castle(const HillEvent *event);
void hill_spaceelevator(const HillEvent *event);
void hill_airstrip(const HillEvent *event);
void hill_hotsprings(const HillEvent *event);
void hill_market(const HillEvent *event);
void hill_lotus(const HillEvent *event);
void hill_bonfire(const HillEvent *event);
void hill_waterfall(const HillEvent *event);
void hill_scarecrows(const HillEvent *event);
void hill_pagoda(const HillEvent *event);
void hill_skaters(const HillEvent *event);
void hill_shepherd(const HillEvent *event);
void hill_bridge(const HillEvent *event);
void hill_totems(const HillEvent *event);
void hill_snowmen(const HillEvent *event);
void hill_sunflowers(const HillEvent *event);
void hill_crystals(const HillEvent *event);
void hill_mushrooms(const HillEvent *event);
void hill_tents(const HillEvent *event);
void hill_fountain(const HillEvent *event);
void hill_harvester(const HillEvent *event);
void hill_cairns(const HillEvent *event);
void hill_watermill(const HillEvent *event);
void hill_marsh(const HillEvent *event);
void hill_stargazers(const HillEvent *event);
void hill_carousel(const HillEvent *event);
void hill_trebuchet(const HillEvent *event);
void hill_orchard(const HillEvent *event);
void hill_radiomast(const HillEvent *event);
void hill_greenhouse(const HillEvent *event);
void hill_swings(const HillEvent *event);
void hill_beehives(const HillEvent *event);
void hill_cableway(const HillEvent *event);
void hill_ruins(const HillEvent *event);
void hill_geodome(const HillEvent *event);
void hill_cabin(const HillEvent *event);
void hill_windchimes(const HillEvent *event);
void hill_picnic(const HillEvent *event);
void hill_wishtree(const HillEvent *event);
void hill_mine(const HillEvent *event);
void hill_eggs(const HillEvent *event);
void hill_hammock(const HillEvent *event);
void hill_telegraph(const HillEvent *event);
void hill_hamlet(const HillEvent *event);
void hill_stepstones(const HillEvent *event);

#endif
