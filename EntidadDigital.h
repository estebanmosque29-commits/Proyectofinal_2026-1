#ifndef ENTIDADDIGITAL_H
#define ENTIDADDIGITAL_H

class EntidadDigital {

protected:
    float velocidadBase;
    bool esActiva;

public:
    EntidadDigital();

    void mover();
    void actualizar(float dt);
    virtual void colisionar();
};

#endif


#ifndef JUGADOR_H
#define JUGADOR_H

#include "EntidadDigital.h"

class Jugador : public EntidadDigital {

};

#endif

#ifndef AGENTEINTELIGENTE_H
#define AGENTEINTELIGENTE_H

#include "EntidadDigital.h"

class AgenteInteligente : public EntidadDigital {

};

#endif


#ifndef PELOTA_H
#define PELOTA_H

class Pelota {

};

#endif

#ifndef GESTORNIVEL_H
#define GESTORNIVEL_H

class GestorNivel {

};

#endif
