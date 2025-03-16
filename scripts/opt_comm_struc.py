from collections import defaultdict, deque
import json
import sys
import math

class Node:
    def __init__(self,num):
        self.parent = None
        self.child = []
        self.num = num
        # self.num = 0

    def add_child(self, child):
        self.child.append(child)

    def checknon(self):
        if self.parent == None and self.child == None:
            return True
        else:
            return False
    
    def print(self, level=0):
        queue = deque([(self, 0)])

        while queue:
            current, level = queue.popleft()
            indent = " " * (level * 2)
            if current.parent != None:
                print(f"{indent}{current.num}(p:{current.parent.num})")
            else:
                print(f"{indent}{current.num}(root)")

            for child in current.child:
                queue.append((child, level + 1))
    
    def exchange(self,n):
        if self.num == n:
            return
        queue = deque([self])
        target_node = None

        while queue:
            current = queue.popleft()
            if current.num == n:
                target_node = current
                break
            for child in current.child:
                queue.append(child)

        if target_node:
            self.num, target_node.num = target_node.num, self.num

    def to_dict(self):
        if self.parent != None and self.child != []:
            return {
                "num": self.num,
                "children": [child.num for child in self.child],
                "parent": self.parent.num
            }
        elif self.child == [] and self.parent == None:
            return {
                "num": self.num,
                # "children": [child.num for child in self.child],
                # "parent": -1
            }
        elif self.child != [] and self.parent == None:
            return {
                "num": self.num,
                "children": [child.num for child in self.child],
                # "parent": 0
            }
        else:
            return {
                "num": self.num,
                # "children": [child.num for child in self.child],
                "parent": self.parent.num
            }                        
        
    def write(self,filename):
        queue = deque([self])
        content = []
        while queue:
            current = queue.popleft()
            content.append(current)
            for child in current.child:
                queue.append(child)
        nodes_list = [node.to_dict() for node in content]
        sorted_data = sorted(nodes_list, key=lambda x: x["num"])
        with open(filename, 'w') as f:
            json.dump(sorted_data, f, indent=4)

def getGraph(tb,ts,n):
    if tb > ts:
        nodes = list()
        for i in range(n):
            nodes.append(Node(i))
        g = list()
        while 1:   
            if n == 1:
                return nodes[0]
            if n%2 == 0:
                for i in range(0,n,2):
                    nodes[i+1].add_child(nodes[i])
                    # nodes[i].num = 1
                    nodes[i].parent = nodes[i+1]
                    g.append(nodes[i+1])
            else:
                for i in range(0,n-3,2):
                    nodes[i+1].add_child(nodes[i])
                    # nodes[i].num = 1
                    nodes[i].parent = nodes[i+1]
                    g.append(nodes[i+1])
                nodes[n-3].parent = nodes[n-1]
                nodes[n-2].parent = nodes[n-1]
                nodes[n-1].add_child(nodes[n-1])
                nodes[n-1].add_child(nodes[n-2])
                g.append(nodes[n-1])
            n = len(g)
            nodes = g
            g = list()
    else:
        k = math.ceil(ts/tb)
        k1 = k+1
        c = ts%tb
        nodes = list()
        for i in range(n):
            nodes.append(Node(i))
        g = list()
        while 1:   
            if n == 1:
                return nodes[0]
            if n%k1 == 0:
                for i in range(0,n,k):
                    nodes[i+1].add_child(nodes[i])
                    # nodes[i].num = 1
                    nodes[i].parent = nodes[i+1]
                    g.append(nodes[i+1])
            else:
                i = 0
                for i in range(0,n-c-k1,k1):
                    for j in range(k-1):
                        nodes[i+j].parent = nodes[i+k-1]
                        nodes[i+k-1].add_child(nodes[i])
                    g.append(nodes[i+1])
                if c == 1:
                    nodes[i+1].add_child(nodes[i])
                    nodes[i].parent = nodes[i+1]
                    g.append(nodes[i+1])
                    nodes[n-3].parent = nodes[n-2]
                    nodes[n-1].parent = nodes[n-2]
                    nodes[n-2].add_child(nodes[n-1])
                    nodes[n-2].add_child(nodes[n-3])
                    g.append(nodes[n-2])
            n = len(g)
            nodes = g
            g = list()
        
            
def getGraph(n):
    nodes = list()
    for i in range(n):
        nodes.append(Node(i))
    g = list()
    while 1:   
        if n == 1:
            return nodes[0]
        if n%2 == 0:
            for i in range(0,n,2):
                nodes[i+1].add_child(nodes[i])
                # nodes[i].num = 1
                nodes[i].parent = nodes[i+1]
                g.append(nodes[i+1])
        else:
            for i in range(0,n-3,2):
                nodes[i+1].add_child(nodes[i])
                # nodes[i].num = 1
                nodes[i].parent = nodes[i+1]
                g.append(nodes[i+1])
            nodes[n-3].parent = nodes[n-2]
            nodes[n-1].parent = nodes[n-2]
            nodes[n-2].add_child(nodes[n-1])
            nodes[n-2].add_child(nodes[n-3])
            g.append(nodes[n-2])
        n = len(g)
        nodes = g
        g = list()

n = int(sys.argv[1])
node = getGraph(n)
# node.print()
node.exchange(n-1)
node.write('graph_'+str(n)+'.json')
# node.print()