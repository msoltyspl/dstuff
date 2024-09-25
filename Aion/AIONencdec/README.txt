
HTML:
the HTML files are encrypted using th following format: the 0th and 1st bit of a byte is XOR-d with the next bit of a
4bits long sequel (meaning that every 4th byte's 0th and 1st bit are XORd with the same bit) the sequel's length is
8bits for the 2nd bit, 16 for the 3rd... and 256 for the 7th one

how the program works (weakpoints) first, as the
encoding for these files are unicode, we can easily get every 2nd byte of the 256 bytes long encryption key (as all
"sequels" are 256/n long, the whole thing repeats after every 256 bytes) as for the rest of the bytes, these html files
starts with 0x81 not encoded, than 0x81 encoded, than 0xff 0xfe to show that its unicode, than an xml header <?xml
version="1.0" encoding="UTF-16" ?> as it is more than 32 bytes long, and we already know every 2nd byte, we have the
first 64 bytes, meaning that we know the 0-5th bits of every encoding byte now all is left to get the last 2 bits

these 2 bits makes 4 different encoding bytes with te 6 known one as the encoding repeats after every 256 byte, every 256th
byte has these 4 possible encoding we try these 4 bytes for all of these bytes that are 256 bytes away from each other,
and look for a one that makes the most English chars(0...1 a...z A...Z .-/<> etc.) the program even considers that the
6th bit repeats after every 128 bytes, decreasing the possibilities even more

for the weakpoints:
1: not translated htmls (0 2nd bytes wont be 0x00)
2: the file starts with something else (not the xml header)
3: the final phase can make minor mistakes, u can manually correct them as mentioned in the begining of this readme
