#!/usr/bin/python

import sys, string, os, struct

#define true and false
true = 1;
false = 0;

#open input file
try:
  inputfilename = sys.argv[1];
  inputfile = open(inputfilename, "r");
except:
  print "Usage: gendocs <inputfile> [<outputfile>]\n"
  sys.exit(1);

#open output file if it exists
try:
  outputfilename = sys.argv[2];
  if (outputfilename != ''):
    outputfile = open(outputfilename, "w");
except:
  outputfilename = '';

#process file
insidedoc = false;
for line in inputfile.readlines():
  splitline = string.split(line)
  try:
    firstword = "%s" % splitline[0]
  except:
    firstword = ''
  if (firstword == '/**'):
    insidedoc = true;
  else:
    if (insidedoc == true):
      if (firstword == '*/'):
        insidedoc = false;
        print "\n\n"
        if (outputfilename != ''):
            outputfile.write("\n\n");
      else:
        if ((firstword == '$SECTION') or (firstword == '$SUBTITLE')):
          print line[(len(firstword) + 1):-1];
          if (outputfilename != ''):
            outputfile.write(line[(len(firstword) + 1):]);
        else:
          print line[:-1];
          if (outputfilename != ''):
            outputfile.write(line);
  
#close input file
inputfile.close()

#close output file if it exists
if (outputfilename != ''):
  outputfile.close();
