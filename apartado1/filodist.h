#define MAX_BOCADOS 10

typedef enum {
     no_sentado=0, queriendo_comer=1,
     comiendo=2, dejando_comer=3,
     pensando=4, queriendo_condimentar=5,
     condimentando=6, dejando_condimentar=7,
     hablando=8} estado_filosofo;