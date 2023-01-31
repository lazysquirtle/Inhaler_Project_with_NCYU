#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "layer_structure.h"
#include "mymodel_API.h"

//0:output   1:conv   2:aver_pool  3:flatten   4:dense   5:lstm
void Create_Input_Layer(LAYER* layer,uint8_t len,uint8_t depth){
    layer->input_layer = (INPUT*)malloc(sizeof(INPUT));
    layer->input_layer->len = len;
    layer->input_layer->depth = depth;
    layer->func = Input_Layer;
    layer->layer_name = (char*)calloc(11,sizeof(char));
    layer->layer_name = "InputLayer";
    layer->class_type_index = 6;
    layer->next_layer = (LAYER*)malloc(sizeof(LAYER));
    layer = layer->next_layer;
    return;
}

void Create_Conv1D_Layer(\
    LAYER* layer,\
    uint8_t filters,\
    uint8_t kernel_size,\
    uint8_t filter_depth,\
    uint8_t input_len,\
    float* weight,\
    float* bias)
{
    layer->conv1d_layer = (CONV1D*)malloc(sizeof(CONV1D));
    layer->conv1d_layer->bias = bias;
    layer->conv1d_layer->weight = weight;
    layer->conv1d_layer->filters = filters;
    layer->conv1d_layer->kernel_size = kernel_size;
    layer->conv1d_layer->filter_depth = filter_depth;
    layer->conv1d_layer->input_len = input_len;
    layer->func = Conv1D_Layer;
    layer->layer_name = (char*)calloc(7,sizeof(char));
    layer->layer_name = "Conv1D";
    layer->class_type_index = 1;
    layer->next_layer = (LAYER*)malloc(sizeof(LAYER));
    layer = layer->next_layer;
    return;
}

void Create_Average_Pooling_1D_Layer(\
    LAYER* layer,\
    uint8_t pool_size,\
    uint8_t input_len,\
    uint8_t input_depth)
{
    layer->average_pooling_1d_layer = (AVERAGE_POOLIND_1D*)malloc(sizeof(AVERAGE_POOLIND_1D));
    layer->average_pooling_1d_layer->pool_size = pool_size;
    layer->average_pooling_1d_layer->input_len = input_len;
    layer->average_pooling_1d_layer->input_depth = input_depth;
    layer->func = Average_Pooling_1D_Layer;
    layer->layer_name = (char*)calloc(17,sizeof(char));
    layer->layer_name = "AveragePolling1D";
    layer->class_type_index = 2;
    layer->next_layer = (LAYER*)malloc(sizeof(LAYER));
    layer = layer->next_layer;
    return;
}


void Create_Dense_Layer(\
    LAYER* layer,\
    float* weight,\
    float* bias,\
    uint8_t input_len,\
    uint8_t output_len,\
    char* activation)
{
    layer->dense_layer = (DENSE*)malloc(sizeof(DENSE));
    layer->dense_layer->weight = weight;
    layer->dense_layer->bias = bias;
    layer->dense_layer->input_len = input_len;
    layer->dense_layer->output_len = output_len;
    layer->dense_layer->activation = activation;
    layer->func = Dense_Layer;
    layer->layer_name = (char*)calloc(6,sizeof(char));
    layer->layer_name = "Dense";
    layer->class_type_index = 4;
    layer->next_layer = (LAYER*)malloc(sizeof(LAYER));
    layer = layer->next_layer;
    return;
}

void Create_LSTM_Layer(\
    LAYER* layer,\
    float* input_weight,\
    float* recurrent_weight,\
    float* bias,\
    uint8_t input_len,\
    uint8_t output_len )
{
    layer->lstm_layer = (LSTM*)malloc(sizeof(LSTM));
    layer->lstm_layer->recurrent = (float*)calloc(output_len,sizeof(float));
    layer->lstm_layer->memarr = (float*)calloc(output_len,sizeof(float));
    layer->lstm_layer->input_weight = input_weight;
    layer->lstm_layer->recurrent_weight = recurrent_weight;
    layer->lstm_layer->bias = bias;
    layer->lstm_layer->input_len = input_len;
    layer->lstm_layer->output_len = output_len;
    layer->func = LSTM_Layer;
    layer->layer_name = (char*)calloc(5,sizeof(char));
    layer->layer_name = "LSTM";
    layer->class_type_index = 5;
    layer->next_layer = (LAYER*)malloc(sizeof(LAYER));
    layer = layer->next_layer;
    return;
}

void Create_Output_Layer(LAYER* layer,uint8_t unit_items){
    layer->output_layer = (OUTPUT*)malloc(sizeof(OUTPUT));
    layer->output_layer->unit_items = unit_items;
    layer->layer_name = (char*)calloc(7,sizeof(char));
    layer->layer_name = "Output";
    layer->class_type_index = 0;
    layer->next_layer = NULL;
    return;
}