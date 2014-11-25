default : all;

run: all
	qemu-system-x86_64 -enable-kvm -nographic --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img

debug:
	(make "DEBUGFLAGS = -g -O0" -C kernel all)
	(make "DEBUGFLAGS = -g -O0" -C user all)
	(make "DEBUGFLAGS = -g -O0" -C fat439 all)
	qemu-system-x86_64 -s -S -nographic --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img

% :
	(make -C kernel $@)
	(make -C user $@)
	(make -C fat439 $@)
