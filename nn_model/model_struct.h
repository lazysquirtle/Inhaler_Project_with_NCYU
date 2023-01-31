// definition of model struct
typedef struct model_t
{
    LAYER *inst;  // instance of a neural network model
    float *input; // input of a model
    int input_dim[2];
    float *output; // output of a model
} model;

typedef struct sliding_window_t
{
    int stride;
} sliding_window;
