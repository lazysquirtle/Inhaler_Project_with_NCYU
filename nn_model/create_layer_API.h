

void Create_Input_Layer(LAYER* layer,uint8_t len,uint8_t depth);
void Create_Conv1D_Layer(\
    LAYER* layer,\
    uint8_t filters,\
    uint8_t kernel_size,\
    uint8_t filter_depth,\
    uint8_t input_len,\
    float* weight,\
    float* bias);
void Create_Average_Pooling_1D_Layer(\
    LAYER* layer,\
    uint8_t pool_size,\
    uint8_t input_len,\
    uint8_t input_depth);
void Create_Dense_Layer(\
    LAYER* layer,\
    float* weight,\
    float* bias,\
    uint8_t input_len,\
    uint8_t output_len,\
    char* activation);
void Create_LSTM_Layer(\
    LAYER* layer,\
    float* input_weight,\
    float* recurrent_weight,\
    float* bias,\
    uint8_t input_len,\
    uint8_t output_len );

void Create_Output_Layer(LAYER* layer,uint8_t unit_items);