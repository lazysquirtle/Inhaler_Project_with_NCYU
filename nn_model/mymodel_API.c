#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "layer_structure.h"
#include "nrf_delay.h"
#include "mem_manager.h"

#define UART_EN 0

// ACTIVATION Function
void relu(float *input){
	if(*input<0)
		*input = 0;
    return;
}

void softmax(float *input , int input_len , double base){
	int index;
    for(index = 0 ; index<input_len ;index++)
		*(input+index) = exp( *(input+index) ) / base ;
    return;
}

void activation_function(char* activation ,float *input , int input_len){
	int index;
    if(!strcmp(activation,"relu"))
		for(index = 0 ; index<input_len ;index++)
			relu(input+index);
	else if(!strcmp(activation,"softmax")){
		double base = 0 ;
		for(index = 0 ; index<input_len ;index++)
			base += exp( *(input+index)  );
		softmax(input,input_len , base);
	}
	else
                #if UART_EN
		printf("No activation_function : %s",activation);
                 #endif
	return;
}

float conv_relu(float* input,float* weight,float* bias,uint8_t kernel_size,uint8_t filter_depth,uint8_t filters_i,uint8_t len_index,uint8_t filters){
    uint8_t depth_i,kernel_i;
    float sum = 0;
    for(depth_i = 0; depth_i < filter_depth; depth_i++){
        for(kernel_i = 0; kernel_i < kernel_size; kernel_i++){
            sum += *(input + (kernel_i + len_index)*filter_depth + depth_i) * *(weight + kernel_i*filter_depth*filters + depth_i * filters + filters_i);
        }
    }
    sum += *(bias + filters_i);
    if(sum < 0) sum = 0;
    return sum;
}

float* Conv1D(  const uint8_t filters, \
                const uint8_t kernel_size,\
                const uint8_t filter_depth,\
                const uint8_t input_len,\
                float* input,\
                float* weight,\
                float* bias)
{
    uint8_t filters_i,len_i,depth_i,kernel_i;
    uint8_t len = input_len - kernel_size + 1;

    float* output;
    output = (float*)nrf_calloc(len*filters,sizeof(float));
    if(output == NULL){
        #if UART_EN
        printf("\r\naver memory is not enough");
        #endif
        while(1);
    }
    
    for(filters_i = 0; filters_i < filters; filters_i++){
        for(len_i = 0; len_i < len; len_i++){
            *(output + len_i*filters + filters_i) = conv_relu(input,weight,bias,kernel_size,filter_depth,filters_i,len_i,filters);
        }
    }
    return output;
}
float* AveragePooling1D(float* input,uint8_t pool_size,uint8_t input_len,uint8_t input_depth){
    uint8_t len_i,depth_i,pool_i;
    float result;
    float* output;
    uint8_t output_len;
    output_len = input_len / pool_size;
    output = (float*)nrf_calloc(output_len*input_depth,sizeof(float));
    if(output == NULL){
        #if UART_EN
        printf("\r\naver memory is not enough");
        #endif
        while(1);
    }
    for(depth_i = 0; depth_i < input_depth; depth_i++){
        for(len_i = 0; input_len - len_i > pool_size; len_i += pool_size){
            result = 0;
            for(pool_i = 0; pool_i < pool_size; pool_i++){
                if((len_i + pool_i) >= input_len) break;
                result += *(input + (len_i + pool_i) * input_depth + depth_i);
            }
        
            result /= (float) pool_i;
            *(output + (len_i / pool_size)*input_depth + depth_i) = result;
        
            //printf("=  %f\r\n",*(output + (len_i / pool_size)*input_depth + depth_i));
            //nrf_delay_ms(70);
        }
    }
    return output;
}

float* Dense(float* input,float* weight,float* bias,uint8_t input_len,uint8_t output_len,char* activation){
    uint8_t input_i,output_i;
    float result;

    float* output;
    output = (float*)nrf_calloc(output_len,sizeof(float));
    if(output == NULL){
        #if UART_EN
        printf("\r\naver memory is not enough");
        #endif
        while(1);
    }
    for(output_i = 0; output_i < output_len; output_i++){
        for(input_i = 0; input_i < input_len; input_i++){
            result += *(input + input_i) * *(weight + input_i*output_len + output_i);
        }
        result += *(bias + output_i);
        *(output + output_i) = result;
        result = 0;
    }
    activation_function(activation ,output , output_len);
    return output;
}

float* LSTM_Conv(float* input,float* weight,uint8_t input_len,uint8_t output_len,uint8_t gate_index){
    uint8_t input_i,output_i;
    float* output;
    float sum;
    output = (float*)nrf_calloc(output_len,sizeof(float));
    if(output == NULL){
        #if UART_EN
        printf("\r\naver memory is not enough");
        #endif
        while(1);
    }
    for(output_i = 0; output_i < output_len; output_i++){
        sum = 0;
        for(input_i = 0; input_i < input_len; input_i++){
            sum += *(input + input_i) * *(weight + (input_i*output_len*4) + gate_index + output_i);
        }
        *(output + output_i) = sum;
    }
    return output;
}

void LSTM_bias(float* input,float* bias,uint8_t len,uint8_t gate_index){
    uint8_t i;
    for(i=0; i<len; i++){
        *(input + i) += *(bias + i + gate_index);
    }
    return;
}

float* arr_plus(float* input1,float* input2,uint8_t len){
    uint8_t len_i;
    float* output;
    output = (float*)nrf_calloc(len,sizeof(float));
    for(len_i = 0; len_i < len; len_i++){
        *(output + len_i) = *(input1 + len_i) + *(input2 + len_i);
    }
    nrf_free(input1);
    nrf_free(input2);
    return output;
}

float* arr_multi(float* input1,float* input2,uint8_t len){
    uint8_t len_i;
    float* output;
    output = (float*)nrf_calloc(len,sizeof(float));
    for(len_i = 0; len_i < len; len_i++){
        *(output + len_i) = *(input1 + len_i) * *(input2 + len_i);
    }
    nrf_free(input1);
    nrf_free(input2);
    return output;
}

void sigmoid(float* input,uint8_t input_len){
    uint8_t len;
    for(len = 0; len < input_len; len++){
        *(input + len) = 1 / (1 + exp(-1 * *(input + len)));
    }
    return;
}

void tanh_fuc(float* input,uint8_t input_len){
    uint8_t len;
    for(len = 0; len < input_len; len++){
        *(input + len) = tanh(*(input + len));
    }
    return;
}
//i f m o
float* Lstm(float* input,
            float* recurrent,
            float* memarr,
            float* input_weight,
            float* recurrent_weight,
            float* bias,
            uint8_t input_len,
            uint8_t output_len)
{
    uint8_t i;
    float* forget_gate;
    float* input_gate;
    float* mem_gate;
    float* output_gate;
    float* output;
    float* temp1;
    float* temp2;
    temp1 = LSTM_Conv(input,input_weight,input_len,output_len,0);
    temp2 = LSTM_Conv(recurrent,recurrent_weight,output_len,output_len,0);
    input_gate = arr_plus(temp1,temp2,output_len);
    LSTM_bias(input_gate,bias,output_len,0);
    sigmoid(input_gate,output_len);

    temp1 = LSTM_Conv(input,input_weight,input_len,output_len,output_len);
    temp2 = LSTM_Conv(recurrent,recurrent_weight,output_len,output_len,output_len);
    forget_gate = arr_plus(temp1,temp2,output_len);
    LSTM_bias(forget_gate,bias,output_len,output_len);
    sigmoid(forget_gate,output_len);

    temp1 = LSTM_Conv(input,input_weight,input_len,output_len,output_len*2);
    temp2 = LSTM_Conv(recurrent,recurrent_weight,output_len,output_len,output_len*2);
    mem_gate = arr_plus(temp1,temp2,output_len);
    LSTM_bias(mem_gate,bias,output_len,output_len*2);
    tanh_fuc(mem_gate,output_len);

    temp1 = LSTM_Conv(input,input_weight,input_len,output_len,output_len*3);
    temp2 = LSTM_Conv(recurrent,recurrent_weight,output_len,output_len,output_len*3);
    output_gate = arr_plus(temp1,temp2,output_len);
    LSTM_bias(output_gate,bias,output_len,output_len*3);
    sigmoid(output_gate,output_len);

    input_gate = arr_multi(input_gate,mem_gate,output_len);
    memarr = arr_multi(memarr,forget_gate,output_len);
    memarr = arr_plus(input_gate,memarr,output_len);
    
    temp1 = (float*)nrf_calloc(output_len,sizeof(float));
    for(i=0;i<output_len;i++)
        *(temp1 + i) = *(memarr + i);
    tanh_fuc(temp1,output_len);
    
    output = arr_multi(temp1,output_gate,output_len);
    for(i=0;i<output_len;i++)
        *(recurrent + i) = *(output + i);
    
    return output;
} 

float* Standardization(float* input,uint8_t len, uint8_t depth){
    uint8_t len_i,depth_i;
    float average,var;
    float* output;
    output = (float*)nrf_calloc(len*depth,sizeof(float));
    for(depth_i = 0; depth_i < depth; depth_i++){
        average = 0;
        var = 0;
        for(len_i = 0; len_i < len; len_i++){
            average += *(input + len_i*depth + depth_i);
            var += *(input + len_i*depth + depth_i) * *(input + len_i*depth + depth_i);
        }
        average /= (float)len;
        var /= (float)len;
        var -= average * average;
        var = sqrt(var);
        #if UART_EN
        printf("\r\naver & var : %f  %f",average,var);
        #endif
        average /= var;
        for(len_i = 0; len_i < len; len_i++){
            *(output + len_i*depth + depth_i) = (*(input + len_i*depth + depth_i) / var) - average; 
        }
    }
    return output;
}
//0:output   1:conv   2:aver_pool  3:flatten   4:dense   5:lstm
void Next_Layer_Input(LAYER *p,float* input){
    switch(p->next_layer->class_type_index)
    {
    case 0:
        p->next_layer->output_layer->output = input;
        break;
     case 1:
        p->next_layer->conv1d_layer->input = input;
        break;
    case 2:
        p->next_layer->average_pooling_1d_layer->input = input;
        break;
    case 4:
        p->next_layer->dense_layer->input = input;
        break;
    case 5:
        p->next_layer->lstm_layer->input = input;
        break;
    default:
        break;
    }
    return;
}


void Input_Layer(LAYER *p){
    INPUT* layer;
    float* output;
    int i,j;
    layer = p->input_layer;
    //output = Standardization(layer->input,layer->len,layer->depth);
    Next_Layer_Input(p,layer->input);
    #if UART_EN
    printf("\r\nInput Layer Done");
    #endif
    nrf_delay_ms(10);
    return;
}

void Conv1D_Layer(LAYER *p){
    CONV1D* layer;
    float* output;
    uint8_t i;
    layer = p->conv1d_layer;
    output = Conv1D(layer->filters,\
                    layer->kernel_size, \
                    layer->filter_depth, \
                    layer->input_len, \
                    layer->input, \
                    layer->weight, \
                    layer->bias);
    Next_Layer_Input(p,output);
    nrf_free(layer->input);
    #if UART_EN
    printf("\r\nConv1D Layer Done");
    #endif
    nrf_delay_ms(10);
    return;
}

void Average_Pooling_1D_Layer(LAYER *p){
    AVERAGE_POOLIND_1D* layer;
    float* output;
    uint8_t i,j;
    layer = p->average_pooling_1d_layer;
    //for(i=0;i<layer->input_len;i++){
    //    for(j=0;j<layer->input_depth;j++){
    //        printf("%f  ",*(layer->input + j + i*layer->input_depth));
    //        nrf_delay_ms(10);
    //    }
    //    printf("\r\n");
    //}

    nrf_delay_ms(10);
    output = AveragePooling1D(layer->input,\
                            layer->pool_size,\
                            layer->input_len,\
                            layer->input_depth);
    Next_Layer_Input(p,output);
    nrf_free(layer->input);
    #if UART_EN
    printf("\r\nAverage Pooling 1D Layer Done");
    #endif
    nrf_delay_ms(10);
    return;
}

void Dense_Layer(LAYER *p){
    DENSE* layer;
    float* output;
    int i;
    layer = p->dense_layer;
    //for(i=0;i<layer->input_len;i++){
    //    printf("%f  ",*(layer->input+i));
    //    nrf_delay_ms(10);
    //}
    output = Dense(layer->input,\
                    layer->weight,\
                    layer->bias,\
                    layer->input_len,\
                    layer->output_len,\
                    layer->activation);
    Next_Layer_Input(p,output);
    nrf_free(layer->input);
    #if UART_EN
    printf("\r\nDense Layer Done");
    #endif
    nrf_delay_ms(10);
    return;
}

void LSTM_Layer(LAYER *p){
    LSTM* layer;
    float* output;
    layer = p->lstm_layer;
    output = Lstm(  layer->input,\
                    layer->recurrent,\
                    layer->memarr,\
                    layer->input_weight,\
                    layer->recurrent_weight,\
                    layer->bias,\
                    layer->input_len,\
                    layer->output_len);
    Next_Layer_Input(p,output);
    nrf_free(layer->input);
    #if UART_EN
    printf("\r\nLSTM Layer Done");
    #endif
    return;
}
