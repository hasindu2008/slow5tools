#ifndef DEGRADE_H
#define DEGRADE_H

#define DNA_EXPERIMENT_TYPE ("genomic_dna")
#define RNA_EXPERIMENT_TYPE ("rna")
#define MINION_DIGITISATION (8192)
#define MINION_DEVICE_TYPE ("minion")
#define PROMETHION_DIGITISATION (2048)
#define PROMETHION_DEVICE_TYPE ("promethion")
#define R10_DNA_SEQUENCING_KIT ("sqk-lsk114")
#define R10_RNA_SEQUENCING_KIT ("sqk-rna004")
#define SAMPLE_FREQUENCY_4KHZ ("4000")
#define SAMPLE_FREQUENCY_5KHZ ("5000")
#define SAMPLING_RATE_4KHZ (4000)
#define SAMPLING_RATE_5KHZ (5000)
#define SLOW5_HEADER_DEVICE_TYPE ("device_type")
#define SLOW5_HEADER_EXPERIMENT_TYPE ("experiment_type")
#define SLOW5_HEADER_SAMPLE_FREQUENCY ("sample_frequency")
#define SLOW5_HEADER_SAMPLE_RATE ("sample_rate")
#define SLOW5_HEADER_SEQUENCING_KIT ("sequencing_kit")
#define SLOW5_HEADER_IS_MISSING(h, a) (!slow5_hdr_get(a, 0, h))

#define DATASET_MINION_R10_DNA \
{\
    MINION_DEVICE_TYPE,\
    SAMPLE_FREQUENCY_5KHZ,\
    "MinION R10 DNA",\
    R10_DNA_SEQUENCING_KIT,\
    DNA_EXPERIMENT_TYPE,\
    MINION_DIGITISATION,\
    SAMPLING_RATE_5KHZ,\
    3,\
}

#define DATASET_PROMETHION_R10_DNA_4KHZ \
{\
    PROMETHION_DEVICE_TYPE,\
    SAMPLE_FREQUENCY_4KHZ,\
    "PromethION R10 DNA 4kHz",\
    R10_DNA_SEQUENCING_KIT,\
    DNA_EXPERIMENT_TYPE,\
    PROMETHION_DIGITISATION,\
    SAMPLING_RATE_4KHZ,\
    3,\
}

#define DATASET_PROMETHION_R10_DNA_5KHZ \
{\
    PROMETHION_DEVICE_TYPE,\
    SAMPLE_FREQUENCY_5KHZ,\
    "PromethION R10 DNA 5kHz",\
    R10_DNA_SEQUENCING_KIT,\
    DNA_EXPERIMENT_TYPE,\
    PROMETHION_DIGITISATION,\
    SAMPLING_RATE_5KHZ,\
    3,\
}

#define DATASET_PROMETHION_R10_RNA \
{\
    PROMETHION_DEVICE_TYPE,\
    SAMPLE_FREQUENCY_4KHZ,\
    "PromethION R10 RNA",\
    R10_RNA_SEQUENCING_KIT,\
    RNA_EXPERIMENT_TYPE,\
    PROMETHION_DIGITISATION,\
    SAMPLING_RATE_4KHZ,\
    3,\
}

struct dataset {
    const char *dev;  // Device type (e.g. "promethion")
    const char *freq; // Sample frequency (e.g. "4000")
    const char *name; // Dataset name
    const char *sqk;  // Sequencing kit (e.g. "sqk-lsk114")
    const char *xpm;  // Experiment type (e.g. "genomic_dna")
    float dig;        // Digitisation (e.g. 2048)
    float rate;       // Sampling rate (e.g. 4000)
    uint8_t q;        // Qts bits suggestion
};

#endif /* degrade.h */
