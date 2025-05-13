# Function Generator
import numpy as np
import matplotlib.pyplot as plt
import random

def Linear(x, y, a, b, c):
    return a * x + b * y + c

def Quadratic(x, y, a, b, c, d, e, f):
    return a * x**2 + b * y**2 + c * x * y + d * x + e * y + f

def Cubic(x, y, a, b, c, d, e, f, g, h, i, j):
    return a*x**3 + b*y**3 + c*x**2*y + d*x*y**2 + e*x**2 + f*y**2 + g*x*y + h*x + i*y + j

def Sine_Wave(x, y, a, b, c, d):
    return a * np.sin(b * x) + c * np.cos(d * y)

def Exponential(x, y, a, b, c):
    return a * np.exp(-b * (x**2 + y**2)) + c

def Polynomial(x, y, *coeffs):
    degree = int(np.sqrt(len(coeffs))) - 1
    result = np.zeros_like(x, dtype=float)
    index = 0
    for i in range(degree + 1):
        for j in range(degree + 1 - i):
            result += coeffs[index] * (x ** i) * (y ** j)
            index += 1
    return result

functions = {
    "Linear": (Linear, ["a", "b", "c"]),
    "Quadratic": (Quadratic, ["a", "b", "c", "d", "e", "f"]),
    "Cubic": (Cubic, ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"]),
    "Sine_Wave": (Sine_Wave, ["a", "b", "c", "d"]),
    "Exponential": (Exponential, ["a", "b", "c"]),
    "Polynomial": (Polynomial, ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o"]),
}


# Select a random function
func_name, (func, params) = random.choice(list(functions.items()))
print(f"Selected function: {func_name}")

# Select a function (manually)
#(func, params) = functions["Cubic"]
#func_name = "Cubic"
#print(f"Selected function: {func_name}")

# Get coefficients from user
coefficients = []
for param in params:
    value = float(input(f"Enter value for {param}: "))
    coefficients.append(value)

# Define grid points (3x3 matrix)
x_vals = np.linspace(-1, 1, 3)
y_vals = np.linspace(-1, 1, 3)
X, Y = np.meshgrid(x_vals, y_vals)
Z = func(X, Y, *coefficients)

# Normalize Z values between 5 and 60
Z_normalized = (Z - np.min(Z)) / (np.max(Z) - np.min(Z)) * 60 + 20


# Display the sampled heights
print("Sampled heights:")
print(np.round(Z_normalized, 2))

# Save each row to the corresponding file in C:\inetpub\wwwroot
file_paths = [
    r"C:\inetpub\wwwroot\01.txt",
    r"C:\inetpub\wwwroot\02.txt",
    r"C:\inetpub\wwwroot\03.txt"
]

for i, row in enumerate(Z_normalized):
    with open(file_paths[i], "w") as file:
        file.write("\n".join(map(lambda x: f"{x:.2f}", row)))
    print(f"Saved row {i+1} to {file_paths[i]}")

# Plot the function surface
fig = plt.figure(figsize=(8, 6))
ax = fig.add_subplot(111, projection='3d')
ax.plot_surface(X, Y, -Z_normalized, cmap='viridis')
ax.set_xlabel('X axis')
ax.set_ylabel('Y axis')
ax.set_zlabel('Height (5-60)')
ax.set_title(f"{func_name} Function Surface")
plt.show()
