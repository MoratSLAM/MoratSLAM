import os
from glob import glob
from setuptools import find_packages, setup

package_name = 'morato'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob(os.path.join('launch', '*launch.[pxy][yma]*'))),
    ],
    install_requires=[
        'setuptools',
        'psutil',
    ],
    zip_safe=True,
    maintainer='Iarley Santos',
    maintainer_email='iarleyconceicaodossantos@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            "get_images = morato.get_images:main",
            "ratslam_visualizer = morato.ratslam_visualizer:main",
            "rasp_perf_monitor = morato.rasp_perf_monitor:main",
        ],
    },
)
