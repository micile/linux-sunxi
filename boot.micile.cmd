setenv bootargs console=ttyS0,115200 loglevel=10 panic=10 earlyprintk debug no_console_suspend clk_ignore_unused
bootz 0x42000000 0x43300000 0x43000000
