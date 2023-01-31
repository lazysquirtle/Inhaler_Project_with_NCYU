#include <stdio.h>
#include <stdlib.h>
#include "layer_structure.h"
#include "mymodel_API.h"
#include "create_layer_API.h"
#include "header.h"

LAYER* load_model(){
   LAYER *layer,*layer_head;
   layer = (LAYER*)malloc(sizeof(LAYER));
   layer_head = layer;
   Create_Input_Layer(layer,150,1);

   layer = layer->next_layer;

   Create_Average_Pooling_1D_Layer(layer,2,150,1);

   layer = layer->next_layer;
   Create_Conv1D_Layer(layer,8,16,1,75,(float*)conv1d_20w,conv1d_20b);

   layer = layer->next_layer;
   Create_Conv1D_Layer(layer,4,8,8,60,(float*)conv1d_21w,conv1d_21b);

   layer = layer->next_layer;
   Create_Average_Pooling_1D_Layer(layer,2,53,4);

   layer = layer->next_layer;
   Create_Dense_Layer(layer,(float*)fc1w,fc1b,104,16,fc1ac);

   layer = layer->next_layer;
   Create_Dense_Layer(layer,(float*)fcw,fcb,16,4,fcac);

   layer = layer->next_layer;
   Create_Output_Layer(layer,4);

   return layer_head;
}


float* predict(float* input,LAYER* head){
   float* output;
   LAYER* layer;
   int units,i;
   layer = head;
   layer->input_layer->input = input;
   printf("\r\n\n================Model Start====================\n")
   while(1){
       if(layer->class_type_index == 0) break;
       layer->func(layer);
       layer = layer->next_layer;
   }

   units = layer->output_layer->unit_items;
   output = (float*)calloc(units,sizeof(float));
   output = layer->output_layer->output;
   for(i=0; i<units; i++){
       printf("\r\n%.2f",*(output + i) * 100);
   }
   printf("\r\n\n=================Model Finish===================")
   return output;
}
