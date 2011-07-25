# Create your views here.
from django.http import HttpResponse
from django.shortcuts import render_to_response
from django.template import RequestContext
from django import forms
from django.contrib import messages


def index(request) :
    return render_to_response('index.html',{},
                               context_instance=RequestContext(request))

def library(request) :
    return render_to_response('library.html', {},
                               context_instance=RequestContext(request))



def results(request) :
    pass



#the session_key for all of spat's data.
def session_key(part) :
    return "spat." + part
