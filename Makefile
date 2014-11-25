default : all;

run: all
	qemu-system-x86_64 -nographic --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img

debug: all
	qemu-system-x86_64 -s -S -nographic --serial mon:stdio -hdc kernel/kernel.img -hdd fat439/user.img

% :
	(make -C kernel $@)
	(make -C user $@)
	(make -C fat439 $@)
