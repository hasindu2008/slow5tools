#ifndef DEGRADE_H
#define DEGRADE_H

#define DNA_EXPERIMENT_TYPE ("genomic_dna")
#define RNA_EXPERIMENT_TYPE ("rna")
#define MINION_DIGITISATION (8192)
#define MINION_DEVICE_TYPE ("minion")
#define MINION_NAME "MinION"
#define GRIDION_DIGITISATION (MINION_DIGITISATION)
#define GRIDION_DEVICE_TYPE ("gridion")
#define GRIDION_NAME "GridION"
#define PROMETHION_DIGITISATION (2048)
#define PROMETHION_DEVICE_TYPE ("promethion")
#define PROMETHION_NAME "PromethION"
#define PROMETHION_2SOLO_DIGITISATION (PROMETHION_DIGITISATION)
#define PROMETHION_2SOLO_DEVICE_TYPE ("p2_solo")
#define PROMETHION_2SOLO_NAME PROMETHION_NAME " 2 Solo"
#define SAMPLE_FREQUENCY_3KHZ ("3000")
#define SAMPLE_FREQUENCY_4KHZ ("4000")
#define SAMPLE_FREQUENCY_5KHZ ("5000")
#define SAMPLING_RATE_3KHZ (3000)
#define SAMPLING_RATE_4KHZ (4000)
#define SAMPLING_RATE_5KHZ (5000)
#define SEQUENCING_KIT_LSK109 ("sqk-lsk109")
#define SEQUENCING_KIT_LSK114 ("sqk-lsk114")
#define SEQUENCING_KIT_RNA002 ("sqk-rna002")
#define SEQUENCING_KIT_RNA004 ("sqk-rna004")
#define SEQUENCING_KIT_ULK114 ("sqk-ulk114")
#define SLOW5_HEADER_DEVICE_TYPE ("device_type")
#define SLOW5_HEADER_EXPERIMENT_TYPE ("experiment_type")
#define SLOW5_HEADER_SAMPLE_FREQUENCY ("sample_frequency")
#define SLOW5_HEADER_SAMPLE_RATE ("sample_rate")
#define SLOW5_HEADER_SEQUENCING_KIT ("sequencing_kit")
#define SLOW5_HEADER_IS_MISSING(h, a) (!slow5_hdr_get(a, 0, h))

#define DATASET_MINION(name, sqk, xpm, freq, rate, q)\
    { name " " MINION_NAME, MINION_DEVICE_TYPE, sqk, xpm, freq, rate,\
      MINION_DIGITISATION, q },\
    { name " " GRIDION_NAME, GRIDION_DEVICE_TYPE, sqk, xpm, freq, rate,\
      GRIDION_DIGITISATION, q }

#define DATASET_PROMETHION(name, sqk, xpm, freq, rate, q)\
    { name " " PROMETHION_NAME, PROMETHION_DEVICE_TYPE, sqk, xpm, freq, rate,\
      PROMETHION_DIGITISATION, q },\
    { name " " PROMETHION_2SOLO_NAME, PROMETHION_2SOLO_DEVICE_TYPE, sqk, xpm,\
      freq, rate, PROMETHION_2SOLO_DIGITISATION, q }

#define DATASETS \
{\
    DATASET_MINION("DNA lsk114 5kHz",\
        SEQUENCING_KIT_LSK114,\
        DNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_5KHZ,\
        SAMPLING_RATE_5KHZ,\
        3),\
    DATASET_PROMETHION("DNA lsk109 4kHz",\
        SEQUENCING_KIT_LSK109,\
        DNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_4KHZ,\
        SAMPLING_RATE_4KHZ,\
        2),\
    DATASET_PROMETHION("DNA lsk114 4kHz",\
        SEQUENCING_KIT_LSK114,\
        DNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_4KHZ,\
        SAMPLING_RATE_4KHZ,\
        3),\
    DATASET_PROMETHION("DNA lsk114 5kHz",\
        SEQUENCING_KIT_LSK114,\
        DNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_5KHZ,\
        SAMPLING_RATE_5KHZ,\
        3),\
    DATASET_PROMETHION("RNA rna002 3kHz",\
        SEQUENCING_KIT_RNA002,\
        RNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_3KHZ,\
        SAMPLING_RATE_3KHZ,\
        2),\
    DATASET_PROMETHION("RNA rna004 4kHz",\
        SEQUENCING_KIT_RNA004,\
        RNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_4KHZ,\
        SAMPLING_RATE_4KHZ,\
        3),\
    DATASET_PROMETHION("DNA ulk114 5kHz",\
        SEQUENCING_KIT_ULK114,\
        DNA_EXPERIMENT_TYPE,\
        SAMPLE_FREQUENCY_5KHZ,\
        SAMPLING_RATE_5KHZ,\
        3),\
}

struct dataset {
    const char *name; // Dataset name
    const char *dev;  // Device type (e.g. "promethion")
    const char *sqk;  // Sequencing kit (e.g. "sqk-lsk114")
    const char *xpm;  // Experiment type (e.g. "genomic_dna")
    const char *freq; // Sample frequency (e.g. "4000")
    float rate;       // Sampling rate (e.g. 4000)
    float dig;        // Digitisation (e.g. 2048)
    uint8_t q;        // Qts bits suggestion
};

#endif /* degrade.h */
