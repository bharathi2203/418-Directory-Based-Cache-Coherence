########################################################################
#   15-418/15-618: Final Project 
#   Implementing Cache Coherence Protocols in Multicore Systems
#
#   generate_traces.py: 
#   - Generate tracefiles to benchmark Cache Coherence Protocols.
#   - Generate tracefiles with different kinds of access patterns.
########################################################################

########################################################################
#   Tracefile format
#   
#   processor_num Read/Write Address 
#   Example:
#   2 R 0x160934985390
#   1 W 0x1614130abc45
#   0 R 0x161b42c84130
#
########################################################################

import random

########################################################################
#   Random Access Pattern 
########################################################################

def generate_random_access_tracefile(n, mem_size, accesses, filename="random.trace"):
    """
    Generate a tracefile simulating random memory access.

    :param n: Number of processors
    :param mem_size: Size of the memory space
    :param accesses: Number of memory accesses
    :param filename: Name of the file to save the trace
    """
    with open(filename, "w") as file:
        for _ in range(accesses):
            proc = random.randint(0, n - 1)
            address = random.randint(0, mem_size - 1) 
            operation = random.choice(["R", "W"])  # Randomly choose Read or Write
            file.write(f"{proc} {operation} {address:016x}\n")

    print(f"Random access tracefile '{filename}' generated for {n} processors.")

# Example usage for 4 processors with a memory space of 1000 elements and 500 accesses
# generate_random_access_tracefile(4, 1000, 500)


########################################################################
#   Parallelized Matrix Multiplication Access Pattern 
########################################################################


def generate_matrix_multiplication_tracefile(m, n, filename="matrix_tracefile.trace"):
    """
    Generate a tracefile simulating the access pattern of the product of two MxM matrices,
    distributed across N processors.

    :param m: Size of the matrix (MxM)
    :param n: Number of processors
    :param filename: Name of the file to save the trace
    """
    elements_per_processor = (m * m) // n  # Elements to be computed per processor

    with open(filename, "w") as file:
        for proc in range(n):
            for _ in range(elements_per_processor):
                # Randomly choose a row and a column to simulate computation
                row = random.randint(0, m-1)
                col = random.randint(0, m-1)

                # Generate Read accesses for the row and column elements
                for i in range(m):
                    # Accessing row element
                    row_address = (row * m + i)   # Assuming each element is 8 bytes
                    file.write(f"{proc} R {row_address:016x}\n")

                    # Accessing column element
                    col_address = (i * m + col)
                    file.write(f"{proc} R {col_address:016x}\n")

                # Generate a Write access for the result element
                result_address = (row * m + col)
                file.write(f"{proc} W {result_address:016x}\n")

    print(f"Matrix multiplication tracefile '{filename}' generated for {m}x{m} matrices across {n} processors.")

# Generate a tracefile for 10x10 matrix multiplication across 4 processors
# generate_matrix_multiplication_tracefile(10, 4)

########################################################################
#   Sequential Access Pattern 
########################################################################

def generate_sequential_access_tracefile(n, length, filename="sequential_64.trace"):
    """
    Generate a tracefile simulating sequential file/array access.

    :param n: Number of processors
    :param length: Length of the file/array
    :param filename: Name of the file to save the trace
    """
    with open(filename, "w") as file:
        for proc in range(n):
            for addr in range(length):
                address = proc * length  + addr   # Assuming each element is 8 bytes
                operation = random.choice(["R", "W"])  # Randomly choose Read or Write
                file.write(f"{proc} {operation} {address:016x}\n")

    print(f"Sequential access tracefile '{filename}' generated for {n} processors.")

# Example usage for 4 processors each accessing an array of length 100
# generate_sequential_access_tracefile(4, 100)


########################################################################
#   Stride Access Patter Tracefile 
########################################################################

def generate_stride_access_tracefile(n, stride, length, filename="stride_tracefile.trace"):
    """
    Generate a tracefile simulating stride access pattern.

    :param n: Number of processors
    :param stride: The stride length between accesses
    :param length: Total number of accesses
    :param filename: Name of the file to save the trace
    """
    with open(filename, "w") as file:
        for proc in range(n):
            for i in range(0, length * stride, stride):
                address = i   # Assuming each element is 8 bytes
                operation = random.choice(["R", "W"])  # Randomly choose Read or Write
                file.write(f"{proc} {operation} {address:016x}\n")

    print(f"Stride access tracefile '{filename}' generated for {n} processors.")

# Example usage for 4 processors, with a stride of 4 and 100 total accesses
# generate_stride_access_tracefile(4, 4, 100)


########################################################################
#    Shared Data in Multi-threaded Applications
########################################################################

def generate_shared_data_access_trace(n, accesses, shared_addresses, filename="shared_data_trace64.trace"):
    """
    This script will simulate multiple processors accessing and modifying a shared data structure.
    """
    with open(filename, "w") as file:
        for _ in range(accesses):
            proc = random.randint(0, n - 1)
            address = random.choice(shared_addresses) 
            operation = random.choice(["R", "W"])
            file.write(f"{proc} {operation} {address:016x}\n")

    print(f"Shared data access tracefile '{filename}' generated.")

# Example usage
#shared_addresses = [100, 101, 102]  # Shared memory addresses
#generate_shared_data_access_trace(4, 500, shared_addresses)

########################################################################
#   Producer-Consumer Pattern
########################################################################

def generate_producer_consumer_trace(n, accesses, buffer_size, filename="producer_consumer_trace.trace"):
    """
    The script simulates producers and consumers accessing a shared buffer.
    """
    with open(filename, "w") as file:
        buffer_addresses = [i for i in range(buffer_size)]
        for _ in range(accesses):
            proc = random.randint(0, n - 1)
            address = random.choice(buffer_addresses) 
            operation = "W" if proc % 2 == 0 else "R"  # Assuming even-numbered are producers
            file.write(f"{proc} {operation} {address:016x}\n")

    print(f"Producer-Consumer tracefile '{filename}' generated.")

# generate_producer_consumer_trace(4, 500, 10)


########################################################################
#   False Sharing
########################################################################


def generate_false_sharing_trace(n, accesses, cache_line_size, filename="false_sharing_trace.trace"):
    """
    This script will simulate false sharing by making different processors
    write to different variables on the same cache line.
    """
    with open(filename, "w") as file:
        # Choose a random start within a cache line
        line_start = random.randint(0, cache_line_size - 1)

        for _ in range(accesses):
            proc = random.randint(0, n - 1)
            # Calculate address within the same cache line
            address = line_start + (proc * cache_line_size) % cache_line_size
            file.write(f"{proc} W {address:016x}\n")

    print(f"False sharing tracefile '{filename}' generated.")

# Example usage
generate_false_sharing_trace(4, 100, 2)

# generate_false_sharing_trace(4, 500, 64)  # Assuming 64-byte cache line


########################################################################
#   Distributed Matrix Operations
########################################################################

def generate_matrix_transpose_trace(m, n, filename="matrix_transpose_trace.trace"):
    """
    The script will simulate a distributed matrix transpose operation.
    """
    with open(filename, "w") as file:
        for row in range(m):
            for col in range(m):
                proc = (row * m + col) % n
                read_address = (row * m + col) 
                write_address = (col * m + row) 
                file.write(f"{proc} R {read_address:016x}\n")
                file.write(f"{proc} W {write_address:016x}\n")

    print(f"Matrix transpose tracefile '{filename}' generated.")

# generate_matrix_transpose_trace(10, 4)


########################################################################
#   Tracefile Generation Calls
########################################################################

#generate_random_access_tracefile(256, 256*4, 50, "random_256.trace") 
generate_matrix_multiplication_tracefile(4, 256, "matrix_256.trace")
generate_sequential_access_tracefile(256, 20, "seq_256.trace")
#generate_false_sharing_trace(256, 1152, 1, "false_sharing256.trace")
#generate_matrix_transpose_trace(10, 256, "transpose_256.trace")
#generate_producer_consumer_trace(256, 2000, 10, "producer_consumer_256.trace")

shared_addresses = [100, 101, 102, 501, 502, 503, 2000, 2001, 2002, 2003]  # Shared memory addresses
#generate_shared_data_access_trace(256, 1000, shared_addresses, "shared_data_256.trace")
