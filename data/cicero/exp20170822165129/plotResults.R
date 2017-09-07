#!/usr/bin/env Rscript
f <- "bench_insert_and_scan_1503497835.csv"
library(plyr)
df = read.csv(f,header=FALSE,strip.white=TRUE,sep=";")
df[7] <- NULL
df[5] <- NULL
names(df) = c("algo","bench","k","time","count")
head(df)

summary(df[df$algo=="GeoHashBinary",])
summary(df[df$algo=="BTree",])
summary(df[df$algo=="RTree",])

summary_avg = ddply(df ,c("algo","k","bench"),summarise,"time"=mean(time))

library(ggplot2)
ggplot(summary_avg, aes(x=k,y=time, color=factor(algo))) + geom_line() + 
facet_wrap(~bench, scales="free",labeller=label_both, ncol=1)

insTime  = subset(summary_avg, bench=="insert")

ggplot(insTime, aes(x=k,y=time, color=factor(algo))) + 
geom_line() +
facet_wrap(~algo, scales="free", ncol=1)

ddply(insTime,c("algo"),summarize, Total=sum(time))

avgTime = cbind(insTime, 
                sumTime=c(lapply(split(insTime, insTime$algo), function(x) cumsum(x$time)), recursive=T),
                avgTime=c(lapply(split(insTime, insTime$algo), function(x) cumsum(x$time)/(x$k+1)), recursive=T)
                )

library(reshape2)
melted_times = melt(avgTime, id.vars = c("algo","k"),measure.vars = c("time","sumTime","avgTime"))

ggplot(melted_times, aes(x=k,y=value,color=factor(algo))) +
geom_line() + 
facet_grid(variable~algo,scales="free", labeller=labeller(variable=label_value))
#facet_wrap(variable~algo,scales="free", labeller=labeller(variable=label_value))
