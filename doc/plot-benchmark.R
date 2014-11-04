# Data
clusters = c(1, 10, 100, 1000)
starcode = c(1, 10, 100, 1000)
seed = c(40, 363, 3701, 37596)
cd.hit = c(49, 448, 3924, 29541)
starcode_pairs = c(1828809738, 277168690, 116797218, 96383875)
slidesort_pairs = c(1722705247, 263551040, 107392689, 87884477)
cluster_sizes = c(1000, 10000, 100000, 1000000)
starcode_time = c(1*60+54, 3*60+23, 7*60+43, 12*60+42)
seed_time = c(51, 43, 37, 30)
cd.hit_time = c(16, 14, 13, 9)
slidesort_time = c(3*60+38, 36*60+47, 384*60+38, 2369*60+1)
starcode_mem = c(2606224, 2420160, 1733536, 1383892)
slidesort_mem = c(388912, 415732, 410948, 371692)
seed_mem = c(2361448, 2320388, 2199632, 2071132)
cd.hit_mem = c(260992, 249052, 247916, 247660)
# Plot
pdf("benchmark.pdf")
par(mfrow=c(2,2))

plot(log10(clusters), log10(starcode), type = "n",
     ylim = c(0,max(log10(c(starcode, cd.hit, seed)))),
     xlab = "Real number of clusters [log10]",
     ylab = "Inferred number of clusters [log10]",
     xaxp = c(0,3,3),
     cex.lab = 1.3)
abline(0,1, col=2)
points(log10(clusters), log10(starcode), pch=1, type="b")
points(log10(clusters), log10(seed), pch=16, type="b")
points(log10(clusters), log10(cd.hit), pch=17, type="b")
legend(x = "topleft", bty="n",
     legend = c("starcode", "seed", "cd-hit"),
     pch = c(1, 16, 17),
     cex = 1.2)
mtext("a", adj=-.05, cex=1.5, line=1)

barplot(slidesort_pairs/starcode_pairs,
     names.arg=log10(clusters),
     xlab = "Number of clusters [log10]",
     ylab = "Pairs found [slidesort/starcode]",
     ylim=c(0,1),
     cex.lab = 1.3)
abline(1,0, col=2)
mtext("b", adj=-.05, cex=1.5, line=1)

plot(log10(cluster_sizes), starcode_time, type = "n",
     ylim = c(1,max(log10(c(starcode_time, cd.hit_time,
                      seed_time, slidesort_time)))),
     xlab = "Cluster size [log10]",
     ylab = "Running time [log10(seconds)]",
     xaxp = c(3,6,3),
     cex.lab = 1.3)
points(log10(cluster_sizes), log10(starcode_time), pch=1, type="b")
points(log10(cluster_sizes), log10(slidesort_time), pch=18, type="b")
points(log10(cluster_sizes), log10(seed_time), pch=16, type="b")
points(log10(cluster_sizes), log10(cd.hit_time), pch=17, type="b")
legend(x = "topleft", bty="n",
     legend = c("starcode", "slidesort", "seed", "cd-hit"),
     pch = c(1, 18, 16, 17),
     cex = 1.2)
mtext("c", adj=-.05, cex=1.5, line=1)
plot(log10(cluster_sizes), starcode_mem/1e6, type = "n",
     ylim = c(0,max(c(starcode_mem, cd.hit_mem,
                      seed_mem, slidesort_mem)/1e6)+0.2),
     xlab = "Cluster size [log10]",
     ylab = "Peak memory usage [GB]",
     xaxp = c(3,6,3),
     cex.lab = 1.3)
points(log10(cluster_sizes), starcode_mem/1e6, pch=1, type="b")
points(log10(cluster_sizes), slidesort_mem/1e6, pch=18, type="b")
points(log10(cluster_sizes), seed_mem/1e6, pch=16, type="b")
points(log10(cluster_sizes), cd.hit_mem/1e6, pch=17, type="b")
mtext("d", adj=-.05, cex=1.5, line=1)
dev.off()
