cmd_arch/arm/lib/strchr.o := /home/ken/workspace/tools/arm-2009q3/bin/arm-none-linux-gnueabi-gcc -Wp,-MD,arch/arm/lib/.strchr.o.d  -nostdinc -isystem /home/ken/workspace/tools/arm-2009q3/bin/../lib/gcc/arm-none-linux-gnueabi/4.4.1/include -I/home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include -Iarch/arm/include/generated -Iinclude  -include include/generated/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-exynos/include -Iarch/arm/plat-s5p/include -Iarch/arm/plat-samsung/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork  -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -gdwarf-2       -funswitch-loops -fpredictive-commoning -fgcse-after-reload -ftree-vectorize -fipa-cp-clone -fsingle-precision-constant -pipe -c -o arch/arm/lib/strchr.o arch/arm/lib/strchr.S

source_arch/arm/lib/strchr.o := arch/arm/lib/strchr.S

deps_arch/arm/lib/strchr.o := \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/linkage.h \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/hwcap.h \
  /home/ken/workspace/exynos4412/exynos4412_Kernel_3.0/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
    $(wildcard include/config/cpu/use/domains.h) \

arch/arm/lib/strchr.o: $(deps_arch/arm/lib/strchr.o)

$(deps_arch/arm/lib/strchr.o):
