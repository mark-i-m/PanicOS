default : all;

% :
	(make -C kernel $@)
	(make -C user $@)
	(make -C fat439 $@)
