library(readr)
library(dplyr)
library(ggplot2)
data <- read_tsv("data/bench/GZFN211103/times", col_names=c("n", "real_time"))
data <- mutate(data, real_time = real_time / 3600)
ggplot(data) +
    aes(x=n, y=real_time) +
    geom_point() +
    labs(x = "Number of Threads", y = "Real time (hrs)") +
    stat_smooth(se = F, method = "gam", formula = y ~ exp(-x))
