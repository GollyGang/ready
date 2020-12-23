# Helper script for understanding stencils and convolutions

from scipy import signal
import numpy as np

# obtain a 2D tri-Laplacian stencil by convolving lap2d with bilap2d
lap2d = np.array([[1,4,1], [4,-20,4], [1,4,1]]) # / 6h^2
bilap2d = np.array([[0,1,1,1,0], [1,-2,-10,-2,1], [1,-10,36,-10,1], [1,-2,-10,-2,1], [0,1,1,1,0]]) # / 3h^4
trilap2d = signal.convolve(lap2d, bilap2d)
print('\n2D tri-Laplacian stencil:')
print('{'+','.join('{'+','.join(str(e) for e in row)+'}' for row in trilap2d)+'}') # format for c++
print('/ 18h^6')

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
print('{'+','.join('{'+','.join('{'+','.join(str(e) for e in row)+'}' for row in plane)+'}' for plane in trilap3d)+'}')
print('/ 1080h^6')
