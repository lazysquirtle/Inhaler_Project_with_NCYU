#include <stdio.h>
#include <stdlib.h>
#include "layer_structure.h"
#include "mymodel_API.h"
#include "create_layer_API.h"
#include "header.h"
#include "inhaler_header.h"
#include "nrf_delay.h"

#define UART_EN 0

LAYER *load_acc_model()
{
    LAYER *layer, *layer_head;
    layer = (LAYER *)malloc(sizeof(LAYER));
    layer_head = layer;
    Create_Input_Layer(layer, 152, 6);

    layer = layer->next_layer;

    Create_Average_Pooling_1D_Layer(layer, 2, 152, 6);

    layer = layer->next_layer;
    Create_Conv1D_Layer(layer, 8, 16, 6, 76, (float *)conv1d_1w, conv1d_1b);

    layer = layer->next_layer;
    Create_Average_Pooling_1D_Layer(layer, 2, 61, 8);

    layer = layer->next_layer;
    Create_Dense_Layer(layer, (float *)fc1w, fc1b, 240, 16, fc1ac);

    layer = layer->next_layer;
    Create_Dense_Layer(layer, (float *)fc2w, fc2b, 16, 8, fc2ac);

    layer = layer->next_layer;
    Create_Dense_Layer(layer, (float *)fcw, fcb, 8, 3, fcac);

    layer = layer->next_layer;
    Create_Output_Layer(layer, 3);

    return layer_head;
}

float *acc_predict(float *input, LAYER *head)
{
    float *output;
    LAYER *layer;
    int units, i;
    layer = head;
    layer->input_layer->input = input;
#if UART_EN
    printf("\r\n\n================Model Start====================\r\n");
#endif
    while (1)
    {
        if (layer->class_type_index == 0)
            break;
        layer->func(layer);
        layer = layer->next_layer;
        nrf_delay_ms(5);
    }

    units = layer->output_layer->unit_items;
    // output = (float*)calloc(units,sizeof(float));
    output = layer->output_layer->output;
#if UART_EN
    for (i = 0; i < units; i++)
    {
        printf("\r\n%.2f", *(output + i) * 100);
    }
    printf("\r\n\n=================ACC Model Finish===================\r\n");
#endif
    return output;
}

LAYER *load_inhaler_model()
{
    LAYER *layer, *layer_head;
    layer = (LAYER *)malloc(sizeof(LAYER));
    layer_head = layer;
    Create_Input_Layer(layer, 150, 2);

    layer = layer->next_layer;

    Create_Average_Pooling_1D_Layer(layer, 2, 150, 2);

    layer = layer->next_layer;
    Create_Conv1D_Layer(layer, 16, 16, 2, 75, (float *)conv1d_26w, conv1d_26b);

    layer = layer->next_layer;
    Create_Average_Pooling_1D_Layer(layer, 2, 60, 16);

    layer = layer->next_layer;
    Create_Conv1D_Layer(layer, 4, 8, 16, 30, (float *)conv1d_27w, conv1d_27b);

    layer = layer->next_layer;
    Create_Dense_Layer(layer, (float *)i_fc1w, i_fc1b, 92, 16, fc1ac);

    layer = layer->next_layer;
    Create_Dense_Layer(layer, (float *)i_fcw, i_fcb, 16, 4, fcac);

    layer = layer->next_layer;
    Create_Output_Layer(layer, 4);

    return layer_head;
}

float *inhaler_predict(float *input, LAYER *head)
{
    float *output;
    LAYER *layer;
    int units, i;
    layer = head;
    layer->input_layer->input = input;
#if UART_EN
    printf("\r\n\n================inhaler Model Start====================\r\n");
#endif
    while (1)
    {
        if (layer->class_type_index == 0)
            break;
        layer->func(layer);
        layer = layer->next_layer;
        nrf_delay_ms(5);
    }

    units = layer->output_layer->unit_items;
    // output = (float*)calloc(units,sizeof(float));
    output = layer->output_layer->output;
#if UART_EN
    for (i = 0; i < units; i++)
    {
        printf("\r\n%.2f", *(output + i) * 100);
    }
    printf("\r\n\n=================inhaler Model Finish===================\r\n");
#endif
    return output;
}
