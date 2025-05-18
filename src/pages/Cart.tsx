import React, { useEffect } from 'react';
import { Link } from 'react-router-dom';
import { Trash2, Plus, Minus, ShoppingBag } from 'lucide-react';
import { useCart } from '../context/CartContext';

const Cart: React.FC = () => {
  const { cart, removeFromCart, updateQuantity, getTotalPrice } = useCart();

  useEffect(() => {
    window.scrollTo(0, 0);
  }, []);

  if (cart.length === 0) {
    return (
      <div className="pt-28 pb-16 min-h-screen">
        <div className="container-custom">
          <div className="text-center py-12">
            <ShoppingBag className="w-16 h-16 mx-auto text-brown-400 mb-4" />
            <h1 className="text-3xl font-playfair font-semibold text-brown-700 mb-4">
              Your Cart is Empty
            </h1>
            <p className="text-brown-500 mb-8">
              Looks like you haven't added any items to your cart yet.
            </p>
            <Link to="/products" className="btn btn-primary">
              Browse Products
            </Link>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="pt-28 pb-16 min-h-screen">
      <div className="container-custom">
        <h1 className="text-3xl font-playfair font-semibold text-brown-700 mb-8">
          Your Shopping Cart
        </h1>

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
          {/* Cart Items */}
          <div className="lg:col-span-2">
            <div className="bg-white rounded-lg shadow-md overflow-hidden">
              <div className="hidden sm:grid grid-cols-5 gap-4 p-4 bg-cream-100 text-brown-700 font-medium">
                <div className="col-span-2">Product</div>
                <div>Price</div>
                <div>Quantity</div>
                <div>Total</div>
              </div>

              {cart.map((item) => (
                <div 
                  key={item.id} 
                  className="grid grid-cols-1 sm:grid-cols-5 gap-4 p-4 border-b border-cream-200 items-center"
                >
                  {/* Product */}
                  <div className="col-span-2 flex items-center gap-4">
                    <img 
                      src={item.image} 
                      alt={item.name} 
                      className="w-16 h-16 object-cover rounded"
                    />
                    <div>
                      <h3 className="font-medium text-brown-700">{item.name}</h3>
                      <p className="text-sm text-brown-500 block sm:hidden">
                        ₹{item.price}
                      </p>
                    </div>
                  </div>

                  {/* Price */}
                  <div className="hidden sm:block text-brown-700">
                    ₹{item.price}
                  </div>

                  {/* Quantity */}
                  <div>
                    <div className="flex items-center border border-cream-300 rounded-md w-fit">
                      <button
                        onClick={() => updateQuantity(item.id, item.quantity - 1)}
                        className="px-2 py-1 text-brown-600 hover:bg-cream-200 transition-colors"
                      >
                        <Minus className="w-4 h-4" />
                      </button>
                      <span className="px-3 py-1 border-l border-r border-cream-300">
                        {item.quantity}
                      </span>
                      <button
                        onClick={() => updateQuantity(item.id, item.quantity + 1)}
                        className="px-2 py-1 text-brown-600 hover:bg-cream-200 transition-colors"
                      >
                        <Plus className="w-4 h-4" />
                      </button>
                    </div>
                  </div>

                  {/* Total */}
                  <div className="flex items-center justify-between">
                    <span className="text-brown-700 font-medium">
                      ₹{(item.price * item.quantity)}
                    </span>
                    <button
                      onClick={() => removeFromCart(item.id)}
                      className="text-red-500 hover:text-red-700 transition-colors"
                    >
                      <Trash2 className="w-5 h-5" />
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Order Summary */}
          <div>
            <div className="bg-white rounded-lg shadow-md p-6">
              <h2 className="text-xl font-playfair font-semibold text-brown-700 mb-4">
                Order Summary
              </h2>
              
              <div className="space-y-3 mb-6">
                <div className="flex justify-between text-brown-600">
                  <span>Subtotal</span>
                  <span>₹{getTotalPrice()}</span>
                </div>
                <div className="flex justify-between text-brown-600">
                  <span>GST (18%)</span>
                  <span>₹{(getTotalPrice() * 0.18)}</span>
                </div>
                <div className="pt-3 border-t border-cream-200 flex justify-between font-semibold text-lg text-brown-700">
                  <span>Total</span>
                  <span>₹{(getTotalPrice() * 1.18)}</span>
                </div>
              </div>
              
              <Link 
                to="/checkout" 
                className="w-full btn btn-primary py-3 text-lg"
              >
                Proceed to Checkout
              </Link>
              
              <div className="mt-6">
                <Link 
                  to="/products" 
                  className="text-brown-600 hover:text-pink-500 transition-colors text-sm flex justify-center"
                >
                  Continue Shopping
                </Link>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Cart