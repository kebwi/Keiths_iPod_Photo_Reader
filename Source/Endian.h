#ifndef __ENDIAN_H__
#define __ENDIAN_H__

bool BigEndianSystem()
{
	int i = 1;
	return ((*(char*)&i) == 0);
}

void FlipEndian(unsigned short &s)
{
	assert(sizeof(unsigned short) == 2);
	
	unsigned short temp;
	char *cp1 = (char*)&temp;
	char *cp2 = ((char*)&s) + 1;
	*cp1 = *cp2;
	cp1++;
	cp2--;
	*cp1 = *cp2;
	s = temp;
}

void FlipEndian(unsigned long &i)
{
	assert(sizeof(unsigned long) == 4);
	
	unsigned long temp;
	char *cp1 = (char*)&temp;
	char *cp2 = ((char*)&i) + 3;
	*cp1 = *cp2;
	cp1++;
	cp2--;
	*cp1 = *cp2;
	cp1++;
	cp2--;
	*cp1 = *cp2;
	cp1++;
	cp2--;
	*cp1 = *cp2;
	i = temp;
}

#endif
