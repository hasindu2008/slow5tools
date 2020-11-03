library(readr)
library(dplyr)
library(ggplot2)
library(ggthemes)
library(tidyverse)
library(gridExtra)

slow5_th <- read_tsv("data/bench/GZFN211103/slow5_extract_times",
                     col_names=c("n", "realtime"))
slow5_has <- read_tsv("data/bench/GZFN211103/fastt.txt", skip = 1,
                     col_names=c("n", "bam", "fasta", "slow5_h", "processing"))
fast5_th <- read_tsv("data/bench/GZFN211103/iot.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_t", "processing"))
fast5_pr <- read_tsv("data/bench/GZFN211103/iop.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_p", "processing"))

slow5_th <- slow5_th %>% mutate(slow5_sas = realtime / 3600) %>% select(-realtime)
#slow5_has <- slow5_has %>% mutate(meth_total = rowSums(.[2:5])) %>% select()
slow5_has <- slow5_has %>% select(n, slow5_h)
#fast5_th <- fast5_th %>% mutate(meth_total = rowSums(.[2:5])) %>% select(fast5)
#fast5_pr <- fast5_pr %>% mutate(meth_total = rowSums(.[2:5])) %>% select(fast5)
fast5_th <- fast5_th %>% select(n, fast5_t)
fast5_pr <- fast5_pr %>% select(n, fast5_p)

data <- merge(merge(merge(slow5_th, slow5_has), fast5_th), fast5_pr)
print(data)

data_gather <- data %>% gather(key="type", value="time", -n)
#data_th_gather <- data_th %>% gather(key="type", value="time", -n)
#total_th_gather <- data_th %>% select(n, slow5, fast5) %>%
#                        gather(key="type", value="time", -n)
#fast5_pr_gather <- fast5_pr %>% gather(key="type", value="time", -n)

ggplot(data_gather) +
    aes(x=n, y=time, color=type) +
    geom_point() +
    geom_line() +
    labs(x = "Number of Threads/Processes", y = "Time (hrs)",
        title = "All Access Times") +
    ylim(0, NA) +
    theme_stata()

ggsave("gpgpu.pdf")

#tot_th <- ggplot(total_th_gather) +
#    aes(x=n, y=time, color=type) +
#    geom_point() +
#    geom_line() +
#    labs(x = "Number of Threads", y = "Real time (hrs)",
#        title = "SLOW5 vs FAST5 Access Time By Number of Threads") +
#    ylim(0, NA) +
#    theme_stata()
#
#fast5_pr <- ggplot(fast5_pr_gather) +
#    aes(x=n, y=time, color=type) +
#    geom_point() +
#    geom_line() +
#    labs(x = "Number of Processes", y = "Real time (hrs)",
#        title = "FAST5 Access Time By Number of Processes") +
#    ylim(0, NA) +
#    theme_stata()

#all_th
#tot_th
#fast5_pr
