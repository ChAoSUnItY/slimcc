void abort();
int strlen(char *str) {
  int i = 0;
  while (str[i]) 
    i++;
  return i;
}
int strcmp(char *s1, char *s2) {
  int i = 0;
  while (s1[i] && s2[i]) 
  {
    if (s1[i] < s2[i]) 
      return -1;
    if (s1[i] > s2[i]) 
      return 1;
    i++;
  }
  return s1[i] - s2[i];
}
int strncmp(char *s1, char *s2, int len) {
  int i = 0;
  while (i < len) 
  {
    if (s1[i] < s2[i]) 
      return -1;
    if (s1[i] > s2[i]) 
      return 1;
    if (!s1[i]) 
      return 0;
    i++;
  }
  return 0;
}
char *strcpy(char *dest, char *src) {
  int i = 0;
  while (src[i]) 
  {
    dest[i] = src[i];
    i++;
  }
  dest[i] = 0;
  return dest;
}
char *strncpy(char *dest, char *src, int len) {
  int i = 0;
  int beyond = 0;
  while (i < len) 
  {
    if (beyond == 0) 
    {
      dest[i] = src[i];
      if (src[i] == 0) 
        beyond = 1;
    }
    else {
      dest[i] = 0;
    }
    i++;
  }
  return dest;
}
char *memcpy(char *dest, char *src, int count) {
  while (count > 0) 
  {
    count--;
    dest[count] = src[count];
  }
  return dest;
}
void __str_base10(char *pb, int val) {
  int neg = 0;
  int q, r, t;
  int i = 15;
  if (val == -2147483648) 
  {
    ;
    return;
  }
  if (val < 0) 
  {
    neg = 1;
    val = -val;
  }
  while (val) 
  {
    q = (val >> 1) + (val >> 2);
    q += (q >> 4);
    q += (q >> 8);
    q += (q >> 16);
    q = q >> 3;
    r = val - (((q << 2) + q) << 1);
    t = ((r + 6) >> 4);
    q += t;
    r = r - (((t << 2) + t) << 1);
    pb[i] += r;
    val = q;
    i--;
  }
  if (neg == 1) 
    pb[i] = 45;
}
