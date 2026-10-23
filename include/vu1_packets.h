#ifndef VU1_PACKETS_H
#define VU1_PACKETS_H

#include "common.h"
#include "model.h"

/*
 * The VU1 programs that draw the models: Vu1_SelectProgram chooses a model's
 * program from the render flags and its mesh's kind. Each program's
 * descriptor names its packet builder (Vu1Pkt_*), which uploads the
 * program's constants (the colours, the matrices, the lights, the fog, the
 * bones' matrices) after the model's part packets and starts it.
 */

void Vu1_SelectProgram(ModelEntry *entry, MeshDesc *mesh);

#endif
