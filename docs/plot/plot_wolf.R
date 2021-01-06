library(readr)
library(dplyr)
library(ggplot2)
library(ggthemes)
library(tidyverse)
library(gridExtra)

slow5_has <- read_tsv("data/bench/GZFN211103/zeta/fastt.txt", skip = 1,
                     col_names=c("n", "bam", "fasta", "slow5", "processing"))
fast5_th <- read_tsv("data/bench/GZFN211103/zeta/iot.txt", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_th", "processing"))
fast5_pr <- read_tsv("data/bench/GZFN211103/zeta/iop.txt", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_pr", "processing"))

slow5_has <- slow5_has %>% select(n, slow5)
fast5_th <- fast5_th %>% select(n, fast5_th)
fast5_pr <- fast5_pr %>% select(n, fast5_pr)

data <- merge(merge(slow5_has, fast5_th), fast5_pr)
print(data)

data_gather <- data %>% gather(key="type", value="time", -n)

ggplot(data_gather) +
    aes(x=n, y=time, color=type) +
    geom_point() +
    geom_line() +
    labs(x = "Number of Threads/Processes", y = "Time (hrs)",
        title = "All Access Times") +
    ylim(0, NA) +
    theme_stata()

ggsave("zeta.pdf")
