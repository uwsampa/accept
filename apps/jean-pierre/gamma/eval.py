import math

def load():
  out = []
  with open('my_output.txt') as f:
    for line in f:
      first_num = line
      out.append(float(first_num))
  return out

def score(orig, relaxed):
  total = 0.0
  for a, b in zip(orig, relaxed):
    if (math.isnan(a) and math.isnan(b)):
      total += 0.0
    elif (math.isnan(a) or math.isnan(b)):
      total += 1.0
    else:
      total += min(abs((a - b) / a), 1.0)
  return total / len(orig)
