# Data
X1 = c(0,1,2,3,4)
Y1 = c(3.15, 4.108, 8.315, 5*60+8.83, 37*60+10.42)
X2 = c(39062, 78125, 156250, 312500, 625000, 1250000, 2500000, 5000000)
Y2 = c(1.735, 6.263, 20.573, 59.309, 2*60+40.555, 6*60+50.695,
      15*60+28.397, 37*60+20.744)
X3 = c(1,2,3,4,6,8,10,12,14,16)
Y3 = c(4*60+48.256, 2*60+24.551, 60+37.476, 60+13.478, 48.613,
       36.442, 28.651, 24.366, 21.665, 19.689)
X4 = 10*c(1,2,3,4,5,6,8,12,16,22,28,34,42)
Y4 = c(2*60+27.740, 4*60+37.854, 4*60+54.134, 5*60+8.237, 60+54.749,
       17.325, 20.589, 23.390, 23.516, 25.902, 26.395, 27.595,
       31.319)
# Plot
pdf("scalability.pdf")
par(mfrow=c(2,2))
plot(log10(X2),log10(Y2), type='b', xlab="Number of sequences [log10]", ylab="Running time [log10(seconds)]", cex.lab=1.3)
fit = lm(log10(Y2)~log10(X2))
abline(fit, col=2)
text(x=6.2, y=3, adj=1,
     sprintf("%.2f+%.2fx", fit$coefficients[1], fit$coefficients[2]))
mtext("a", adj=-.05, cex=1.5, line=1)
plot(X1,Y1, type='b', xlab="Levenshtein distance", ylab="Running time [seconds]", cex.lab=1.3)
mtext("b", adj=-.05, cex=1.5, line=1)
plot(X4,Y4, ylim=1.05*c(0,max(Y4)), type='b', xlab="Sequence length", ylab="Running time [seconds]", cex.lab=1.3)
mtext("c", adj=-.05, cex=1.5, line=1)
plot(X3,Y3[1]/Y3, type='b', ylim=c(1,16), xlim=c(1,16), xlab="Number of threads", ylab="Relative performance", cex.lab=1.3)
abline(0,1, col=2)
mtext("d", adj=-.05, cex=1.5, line=1)
dev.off()

