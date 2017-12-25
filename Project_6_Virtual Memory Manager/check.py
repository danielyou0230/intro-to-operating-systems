import numpy as np
f_ans = open("correct.txt", "r")
f_out = open("output.txt", "r")
correct = True
count = 0
for i in range(1000):
	passed = True
	#
	buff = f_ans.readline().split()
	ans_a = int(buff[-3])
	ans_v = int(buff[-1])
	buff = f_out.readline().split()
	out_a = int(buff[-4])
	out_v = int(buff[-1])
	#
	if (out_a != ans_a):
		print "[REQUEST {:4d}] ANS {:5d} {:5d} | OUT addr: {:5d} value {:5d}".format(i + 1, ans_a, ans_v, out_a, out_v)
		correct = False
		passed = False
	#
	if (out_v != ans_v):
		print "[REQUEST {:4d}] ANS {:5d} {:5d} | OUT addr: {:5d} value {:5d}".format(i + 1, ans_a, ans_v, out_a, out_v)
		correct = False
		passed = False
	#
	if (passed == False):
		count += 1

if (correct):
	print "All case passed"
else:
	print "Failed on {:4d} cases".format(count)
