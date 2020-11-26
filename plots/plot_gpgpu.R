library(readr)
library(dplyr)
library(ggplot2)
library(ggthemes)
library(tidyverse)
library(gridExtra)
library(plotly)
library(htmlwidgets)

blow5_th <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/blow5_times",
                     col_names=c("n", "blow5"))
clow5_th <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/clow5_times-2",
                     col_names=c("n", "clow5"))
slow5_th <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/slow5_times",
                     col_names=c("n", "slow5_sas"))
slow5_has <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/slow5_iot.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "slow5_h", "processing"))
fast5_th <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/fast5_iot.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_t", "processing"))
fast5_pr <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/fast5_iop.tsv", skip = 1,
                     col_names=c("n", "bam", "fasta", "fast5_p", "processing"))
sizes <- read_tsv("../tests/data/bench/GZFN211103/gpgpu/sizes.tsv",
                     col_names=c("size", "filetype"))

read_len <- parse_number(system("wc -l ../tests/data/bench/GZFN211103/reads.list", intern=TRUE))

blow5_th <- blow5_th %>% mutate(blow5 = blow5 / 3600)
clow5_th <- clow5_th %>% mutate(clow5 = clow5 / 3600)
slow5_th <- slow5_th %>% mutate(slow5_sas = slow5_sas / 3600)
slow5_has <- slow5_has %>% select(n, slow5_h)
fast5_th <- fast5_th %>% select(n, fast5_t)
fast5_pr <- fast5_pr %>% select(n, fast5_p)


# All data plotted together

data <- blow5_th %>%
    merge(clow5_th) %>%
    merge(slow5_th) %>%
    merge(slow5_has) %>%
    merge(fast5_th) %>%
    merge(fast5_pr)
names(data) <- c("n",
                 "SLOW5 Binary (MT)",
                 "SLOW5 Compressed (MT)",
                 "SLOW5 ASCII (MT)",
                 "SLOW5 ASCII (MT-Hasindu)",
                 "FAST5 (MT)",
                 "FAST5 (MP)")

data_gather <- data %>% gather(key="File Type (Method)", value="time", -n)
data_gather <- data_gather %>% mutate(reads_per_time = read_len / (time * 3600))

p1 <- ggplot(data_gather) +
    geom_line(aes(x=n, y=reads_per_time, color=`File Type (Method)`)) +
    geom_point(aes(x=n, y=reads_per_time, color=`File Type (Method)`,
                   text=paste0(`File Type (Method)`, "<br>",
                               n, ifelse(grepl("MT", `File Type (Method)`),
                                         " threads",
                                         " processes"), "<br>",
                               "Avg ", round(reads_per_time, 0), " reads/sec"))) +
    labs(x = "Number of Threads/Processes",
         y = "Average Number of Reads Accessed per Second (avg reads/sec)",
         title = "Reads Accessed Per Second on Average Against Number of Threads/Processes") +
    ylim(0, NA)

ggsave("gpgpu_read_time.pdf", p1)
p2 <- ggplotly(p1, tooltip="text")
saveWidget(p2, "gpgpu_read_time.html")

# Bar plot all for 1 thread/process

data_gather_1 <- data_gather[data_gather$n == 1,]
p3 <- ggplot(data_gather_1) +
    aes(x=fct_reorder(`File Type (Method)`, reads_per_time), y=reads_per_time, fill=`File Type (Method)`) +
    geom_bar(stat="identity", position="dodge") +
    labs(x = "File Type (Method)",
         y = "Average Reads Accessed per Second (avg reads/sec)",
         title = "Reads Accessed Per Second on Average Using 1 Thread/Process")

ggsave("gpgpu_read_time_1.pdf", p3)
p4 <- ggplotly(p3)
saveWidget(p4, "gpgpu_read_time_1.html")


# Threaded data only

data_th <- blow5_th %>%
    merge(clow5_th) %>%
    merge(slow5_th) %>%
    merge(fast5_th)
names(data_th) <- c("n",
                 "SLOW5 Binary",
                 "SLOW5 Compressed",
                 "SLOW5 ASCII",
                 "FAST5")


data_gather_th <- data_th %>% gather(key="File Type", value="time", -n)
data_gather_th <- data_gather_th %>% mutate(reads_per_time = read_len / (time * 3600))

p1_th <- ggplot(data_gather_th) +
    geom_line(aes(x=n, y=reads_per_time, color=`File Type`)) +
    geom_point(aes(x=n, y=reads_per_time, color=`File Type`,
                   text=paste0(`File Type`, "<br>",
                               n, " threads<br>",
                               "Avg ", round(reads_per_time, 0), " reads/sec"))) +
    labs(x = "Number of Threads",
         y = "Average Number of Reads Accessed per Second (avg reads/sec)",
         title = "Reads Accessed Per Second on Average Against Number of Threads") +
    ylim(0, NA)

ggsave("gpgpu_read_time_thread.pdf", p1_th)
p2_th <- ggplotly(p1_th, tooltip="text")
saveWidget(p2_th, "gpgpu_read_time_thread.html")



# File size plot

sizes$size <- sizes$size / read_len / (2^(10))

sizes <- sizes[!grepl("index", sizes$filetype),]

p1_sz <- ggplot(sizes) +
    aes(x=fct_reorder(filetype, size), y=size, fill=filetype) +
    geom_bar(stat="identity", position="dodge") +
    labs(x = "File Type",
         y = "Kilobytes per Read on Average",
         title = "File Size Per Read on Average")

ggsave("gpgpu_size.pdf", p1_sz)
p2_sz <- ggplotly(p1_sz)
saveWidget(p2_sz, "gpgpu_size.html")
