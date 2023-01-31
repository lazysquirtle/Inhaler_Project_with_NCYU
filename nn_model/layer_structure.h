#include <stdint.h>

#ifndef __LAYER_STRUCTURE_H__
#define __LAYER_STRUCTURE_H__

typedef struct conv1d_s {
  uint8_t filters;
  uint8_t kernel_size;
  uint8_t filter_depth;
  uint8_t input_len;
  float *input;
  float *weight;
  float *bias;
} CONV1D;

typedef struct dense_s {
  float *input;
  float *weight;
  float *bias;
  uint8_t input_len;
  uint8_t output_len;
  char *activation;
} DENSE;

typedef struct average_pooling_1d_s {
  float *input;
  uint8_t pool_size;
  uint8_t input_len;
  uint8_t input_depth;
} AVERAGE_POOLIND_1D;

typedef struct lstm_s {
  float *input;
  float *recurrent;
  float *memarr;
  float *input_weight;
  float *recurrent_weight;
  float *bias;
  uint8_t input_len;
  uint8_t output_len;
} LSTM;

typedef struct input_s {
  float *input;
  uint8_t len;
  uint8_t depth;
} INPUT;

typedef struct output_s {
  float *output;
  uint8_t unit_items;
} OUTPUT;

typedef struct layer_s {
  char *layer_name;
  int class_type_index;
  INPUT *input_layer;
  CONV1D *conv1d_layer;
  AVERAGE_POOLIND_1D *average_pooling_1d_layer;
  DENSE *dense_layer;
  LSTM *lstm_layer;
  OUTPUT *output_layer;
  void (*func)(struct layer_s *);
  struct layer_s *next_layer;
} LAYER;

#endif