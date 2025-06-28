build:
	cargo build

run:
	cargo build 
	qemu-system-riscv32 -nographic -machine virt -net none   -chardev stdio,id=con,mux=on \
	-serial chardev:con   -mon chardev=con,mode=readline -bios none   -smp 4 \
	-kernel ./target/riscv32imac-unknown-none-elf/debug/hexarch_freeRTOS_rust


debug:
	cargo build 
	qemu-system-riscv32 -nographic -machine virt -net none   -chardev stdio,id=con,mux=on \
	-serial chardev:con   -mon chardev=con,mode=readline -bios none   -smp 4 \
	-kernel ./target/riscv32imac-unknown-none-elf/debug/hexarch_freeRTOS_rust -s -S

remote:
	gdb-multiarch -q ./target/riscv32imac-unknown-none-elf/debug/hexarch_freeRTOS_rust

doc:
	cargo doc --no-deps