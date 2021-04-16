#!/usr/bin/Rscript

library(readr)
library(dplyr)
library(ggplot2)
library(ggthemes)
library(tidyverse)
library(gridExtra)
library(plotly)
library(htmlwidgets)

slow5 <- read_tsv("slow5_times", col_names=c("n", "slow5"))
blow5 <- read_tsv("blow5_times", col_names=c("n", "blow5"))
clow5 <- read_tsv("clow5_times", col_names=c("n", "clow5"))
fast5 <- read_tsv("fast5_iot.tsv", skip = 1,
                  col_names=c("n", "bam", "fasta", "fast5_t", "processing"))

sizes <- read_tsv("sizes.tsv", col_names=c("size", "filetype"))

list_nreads <- parse_number(system("wc -l reads.list", intern=TRUE))
list_nbases <- parse_number(system("cat list_nbases", intern=TRUE))
nbases <- parse_number(system("cat nbases", intern=TRUE))

# Select just the fast5 thread access times in seconds
fast5 <- fast5 %>% select(n, fast5_t) %>% mutate(fast5_t = fast5_t * 3600)
# Combine the times data
data <- slow5 %>%
    merge(blow5) %>%
    merge(clow5) %>%
    merge(fast5)
names(data) <- c("n",
                 "SLOW5",
                 "BLOW5 (no)",
                 "BLOW5 (gzip)",
                 "FAST5")

# Make dataframe lengthwise
data_gather <- data %>% gather(key="File Type", value="time", -n)
# Add megabases per second column
data_gather <- data_gather %>% mutate(bases_per_time = (list_nbases / (10^6)) / time)

# Megabases accessed VS Num threads
p1_time <- ggplot(data_gather) +
    geom_line(aes(x=n, y=bases_per_time, color=`File Type`)) +
    geom_point(aes(x=n, y=bases_per_time, color=`File Type`,
                   text=paste0(`File Type`, "<br>",
                               n, " threads<br>",
                               "Avg ", round(bases_per_time,2), " Mbases/sec"))) +
    labs(x = "Number of Threads",
         y = "Average Megabases Accessed per Second (avg. Mbases/s)") +
    ylim(0, NA) +
    theme_bw(base_family="Helvetica", base_size=14) +
    theme(legend.position="top", legend.direction="vertical") +
    scale_color_brewer(palette="Dark2")
ggsave("bases_read_vs_threads.pdf", p1_time)
ggsave("bases_read_vs_threads.png", p1_time)
p2_time <- ggplotly(p1_time, tooltip="text")
saveWidget(p2_time, "bases_read_vs_threads.html")

# Remove index files
sizes <- sizes[!grepl("idx|slow5_total", sizes$filetype),]
# Rename
sizes$filetype[sizes$filetype == "bench.slow5"] <- "SLOW5"
sizes$filetype[sizes$filetype == "bench.blow5"] <- "BLOW5 (no)"
sizes$filetype[sizes$filetype == "bench_gz.blow5"] <- "BLOW5 (gzip)"
sizes$filetype[sizes$filetype == "fast5_total"] <- "FAST5"
# Change from bytes to bytes/base
sizes$size <- sizes$size / nbases

# File size per base plot
p1_sz <- ggplot(sizes) +
    geom_bar(aes(x=fct_reorder(filetype, size), y=size, fill=filetype,
                 text=paste0("Avg ", round(size,2), " B/base")),
             stat="identity", position="dodge") +
    labs(x = "File Type",
         y = "Bytes per Base on Average (avg. B/base)") +
    theme_bw(base_family="Helvetica", base_size=14) +
    guides(fill=FALSE) +
    scale_fill_brewer(palette="Dark2")
ggsave("size_per_base.pdf", p1_sz)
ggsave("size_per_base.png", p1_sz)
p2_sz <- ggplotly(p1_sz, tooltip="text")
saveWidget(p2_sz, "size_per_base.html")
