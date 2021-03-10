import numpy as np
from matplotlib import pyplot as plt

num_samples = 20000
num_lines = 200
ADvalue = []

def main():
    with open("data.txt",'r') as f:
        for i in range(num_lines):
            for x in f.readline().split(','):
                if x != '\n':
                    ADvalue.append(int(x))
        time = int(f.readline())
    plt.plot(np.linspace(0,time,num_samples),ADvalue)
    plt.show()

if __name__ == "__main__":
    main()
