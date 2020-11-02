library(readr)
library(dplyr)
library(ggplot2)
library(ggthemes)
library(tidyverse)
library(gridExtra)

slow5_th <- read_tsv("data/bench/GZFN211103/slow5_extract_times",
                     col_names=c("n", "realtime"))
fast5_th <- read_tsv("data/bench/GZFN211103/iot.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5", "processing"))
fast5_pr <- read_tsv("data/bench/GZFN211103/iop.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5", "processing"))

slow5_th <- slow5_th %>% mutate(slow5_total = realtime / 3600) %>% select(-realtime)
fast5_th <- fast5_th %>% mutate(fast5_total = rowSums(.[2:5]))
fast5_pr <- fast5_pr %>% mutate(fast5_total = rowSums(.[2:5]))

data_th <- merge(slow5_th, fast5_th)

data_th_gather <- data_th %>% gather(key="type", value="time", -n)
total_th_gather <- data_th %>% select(n, slow5_total, fast5_total) %>%
                        gather(key="type", value="time", -n)
fast5_pr_gather <- fast5_pr %>% gather(key="type", value="time", -n)

all_th <- ggplot(data_th_gather) +
    aes(x=n, y=time, color=type) +
    geom_point() +
    geom_line() +
    labs(x = "Number of Threads", y = "Real time (hrs)",
        title = "All Access Times By Number of Threads") +
    ylim(0, NA) +
    theme_stata()

tot_th <- ggplot(total_th_gather) +
    aes(x=n, y=time, color=type) +
    geom_point() +
    geom_line() +
    labs(x = "Number of Threads", y = "Real time (hrs)",
        title = "SLOW5 vs FAST5 Access Time By Number of Threads") +
    ylim(0, NA) +
    theme_stata()

fast5_pr <- ggplot(fast5_pr_gather) +
    aes(x=n, y=time, color=type) +
    geom_point() +
    geom_line() +
    labs(x = "Number of Processes", y = "Real time (hrs)",
        title = "FAST5 Access Time By Number of Processes") +
    ylim(0, NA) +
    theme_stata()

all_th
tot_th
fast5_pr
