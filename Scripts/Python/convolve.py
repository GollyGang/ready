# Helper script for understanding stencils and convolutions

from scipy import signal
from scipy import ndimage
import numpy as np

def format1dforcplusplus(arr):
    return '{' + ','.join(str(round(e)) for e in arr) + '}'

def format2dforcplusplus(arr):
    return '{' + ','.join(format1dforcplusplus(e) for e in arr) + '}'

def format3dforcplusplus(arr):
    return '{' + ','.join('\n'+format2dforcplusplus(e) for e in arr) + '}'

# obtain a 2D tri-Laplacian stencil by convolving lap2d with bilap2d
lap2d = np.array([[1,4,1], [4,-20,4], [1,4,1]]) # / 6h^2
bilap2d = np.array([[0,1,1,1,0], [1,-2,-10,-2,1], [1,-10,36,-10,1], [1,-2,-10,-2,1], [0,1,1,1,0]]) # / 3h^4
trilap2d = signal.convolve(lap2d, bilap2d)
print('\n2D tri-Laplacian stencil:')
print(format2dforcplusplus(trilap2d), '/ 18h^6')

# obtain a 3D tri-Laplacian stencil by convolving lap3d with bilap3d
lap3d = np.array([[[1,3,1,],[3,14,3,],[1,3,1,],],
                  [[3,14,3,],[14,-128,14,],[3,14,3,],],
                  [[1,3,1,],[3,14,3,],[1,3,1,],],]) # / 30h^2
bilap3d = np.array([[[-1,0,0,0,-1,],[0,10,0,10,0,],[0,0,0,0,0,],[0,10,0,10,0,],[-1,0,0,0,-1,],],
                    [[0,10,0,10,0,],[10,-20,-36,-20,10,],[0,-36,0,-36,0,],[10,-20,-36,-20,10,],[0,10,0,10,0,],],
                    [[0,0,0,0,0,],[0,-36,0,-36,0,],[0,0,360,0,0,],[0,-36,0,-36,0,],[0,0,0,0,0,],],
                    [[0,10,0,10,0,],[10,-20,-36,-20,10,],[0,-36,0,-36,0,],[10,-20,-36,-20,10,],[0,10,0,10,0,],],
                    [[-1,0,0,0,-1,],[0,10,0,10,0,],[0,0,0,0,0,],[0,10,0,10,0,],[-1,0,0,0,-1,],],]) # / 36h^4
trilap3d = signal.convolve(lap3d, bilap3d)
print('\n3D tri-Laplacian stencil:')
print(format3dforcplusplus(trilap3d), '/ 1080h^6')

# obtain Gaussian stencils by gaussian filtering a single pixel
# get reasonable results but differs from well-known results, perhaps
# because is point-sampling rather than taking an integral over each pixel?
'''
input = np.zeros(5)
input[2]=1
gauss1d = ndimage.gaussian_filter(input,1,mode='constant',cval=0)
divisor = round(1 / gauss1d[0])
gauss1d = np.round(gauss1d * divisor)
print('\n1D Gaussian stencil:')
print(format1dforcplusplus(gauss1d), '/', divisor)

input = np.zeros((5,5))
input[2][2]=1
gauss2d = ndimage.gaussian_filter(input,1,mode='constant',cval=0)
divisor = round(1 / gauss2d[0][0])
gauss2d = np.round(gauss2d * divisor)
print('\n2D Gaussian stencil:')
print(format2dforcplusplus(gauss2d), '/', divisor)

input = np.zeros((5,5,5))
input[2][2][2]=1
gauss3d = ndimage.gaussian_filter(input,1,mode='constant',cval=0)
divisor = round(1 / gauss3d[0][0][0])
gauss3d = np.round(gauss3d * divisor)
print('\n3D Gaussian stencil:')
print(format3dforcplusplus(gauss3d), '/', divisor)
'''

# alternative method for gaussians:
# starting from the well-known 1/273 2d kernel: https://homepages.inf.ed.ac.uk/rbf/HIPR2/gsmooth.htm
gauss1d = np.array([17,66,107,66,17]) # sum of columns
divisor = gauss1d.sum()
print('\n1D Gaussian stencil:')
print(format1dforcplusplus(gauss1d), '/', divisor)

# show the 2d version, for checking
gauss2d = np.round(np.outer(gauss1d,gauss1d))
gauss2d = np.round(gauss2d / 276)
divisor = gauss2d.sum()
print('\n2D Gaussian stencil:')
print(format2dforcplusplus(gauss2d), '/', divisor)

gauss3d = np.round(np.multiply.outer(gauss2d,gauss1d))
gauss3d = np.round(gauss3d / gauss3d[0][0][0])
divisor = gauss3d.sum()
print('\n3D Gaussian stencil:')
print(format3dforcplusplus(gauss3d), '/', divisor)
